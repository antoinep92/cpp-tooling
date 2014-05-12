#ifndef REPLACEREFS_H
#define REPLACEREFS_H

#include "clang/Lex/Lexer.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/CommonOptionsParser.h"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <boost/algorithm/string.hpp>
template<class C> using const_range = boost::iterator_range<typename C::const_iterator>;
template<class C> using couple = std::pair<C,C>;

struct ReplaceRefs : clang::ASTFrontendAction {

	static std::string shortName(const std::string & qualified) {
		std::list<const_range<std::string>> splits;
		boost::split(splits, qualified, [&](char c){ return c == ':'; }, boost::token_compress_on);
		return { splits.back().begin(), splits.back().end() };
	}
	static couple<int> align(const std::string & word, const std::string & text) {
		using R = const_range<std::string>;
		std::list<R> matches;
		boost::find_all(matches, text, word);
		assert(matches.size() == 1);
		R & match = matches.front();
		return {
			match.begin() - text.begin(),
			text.size() - (match.end() - text.begin())
		};
	}

	using Renames = std::unordered_map<std::string /* qualified original name */, std::string /* unqualified target name */>;
	Renames renames;
	clang::tooling::Replacements & replacements;
	static const bool save = false;
	ReplaceRefs(const Renames & renames, clang::tooling::Replacements & replacements) :
		renames(renames), replacements(replacements) {}

	struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {
		struct Rename {
			std::string originalQualified, originalShort;
			std::string newName;
			void operator=(const couple<std::string> & strings) {
				originalQualified = strings.first;
				originalShort = shortName(strings.first);
				newName = strings.second;
			}
		};
		struct Item {
			Rename rename;
			std::unordered_set<clang::Decl*> declarations;
			std::unordered_set<clang::DeclRefExpr*> references;
		};
		std::unordered_map<clang::Decl*, Item> index;

		const Renames & renames;
		clang::tooling::Replacements & replacements;
		clang::SourceManager & sourceManager;

		Impl(const Renames & renames, clang::tooling::Replacements & replacements, clang::SourceManager & sourceManager) :
			renames(renames), replacements(replacements), sourceManager(sourceManager) {}

		void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
			std::cout << "Scanning AST..." << std::endl;
			TraverseDecl(context.getTranslationUnitDecl());
			//dump();
			addReplacements();
		}
		bool VisitDecl(clang::Decl * decl) { // from RecursiveASTVisitor
			if(llvm::dyn_cast<clang::NamedDecl>(decl)) {
				auto iter = renames.find(static_cast<clang::NamedDecl*>(decl)->getQualifiedNameAsString());
				if(iter != renames.end()) {
					auto & item = index[decl->getCanonicalDecl()];
					item.rename = *iter;
					item.declarations.insert(decl);
				}
			}
			return true;
		}

		bool VisitStmt(clang::Stmt * stmt) { // from RecursiveASTVisitor
			if(llvm::dyn_cast<clang::DeclRefExpr>(stmt)) {
				auto iter = index.find(static_cast<clang::DeclRefExpr*>(stmt)->getDecl()->getCanonicalDecl());
				if(iter != index.end()) iter->second.references.insert(static_cast<clang::DeclRefExpr*>(stmt));
			}
			return true;
		}

		template<class X> llvm::StringRef nodeText(const X * xpr) const {
			auto start = sourceManager.getSpellingLoc(xpr->getLocStart()), end = sourceManager.getSpellingLoc(xpr->getLocEnd());
			return {
				sourceManager.getCharacterData(start),
				sourceManager.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(end, 0, sourceManager, clang::LangOptions())).second
				- sourceManager.getDecomposedLoc(start).second
			};
		}

		template<class X> void addReplacement(const X * xpr, const std::string & original, const std::string & replacement) {
			auto text = nodeText(xpr);
			auto range = clang::CharSourceRange::getTokenRange(xpr->getSourceRange());
			auto p = align(original, text);
			std::cout << '\t' << std::string(text.begin() + p.first, text.end() - p.second) << "  ==>  " << replacement << std::endl;
			clang::CharSourceRange target{
				clang::SourceRange(range.getBegin().getLocWithOffset(p.first), range.getEnd().getLocWithOffset(-p.second)),
				true
			};
			replacements.emplace(sourceManager, target, llvm::StringRef(replacement));
		}
		void addReplacements() {
			std::cout << "computing patch..." << std::endl;
			for(auto & ip : index) {
				for(auto decl : ip.second.declarations) addReplacement(decl, ip.second.rename.originalShort, ip.second.rename.newName);
				for(auto ref : ip.second.references) addReplacement(ref, ip.second.rename.originalShort, ip.second.rename.newName);
			}

		}

		void dump() const {
			for(auto & ip : index) {
				std::cout << ip.second.rename.originalQualified << " ==> " << ip.second.rename.newName << std::endl;
				std::cout << '\t' << "cannonical declaration : " << ip.first << std::endl;
				std::cout << '\t' << "declarations:" << std::endl;
				for(auto decl : ip.second.declarations) std::cout << "\t\t" << decl << std::endl;
				std::cout << '\t' << "referencing expressions:" << std::endl;
				for(auto ref : ip.second.references) std::cout << "\t\t" << ref << std::endl;
			}
		}
	}; // Impl

	static clang::tooling::FrontendActionFactory * newFactory(const Renames & renames, clang::tooling::Replacements & replacements) {
			struct Factory : clang::tooling::FrontendActionFactory {
					const Renames & renames;
					clang::tooling::Replacements & replacements;
					Factory(const Renames & renames, clang::tooling::Replacements & replacements) : renames(renames), replacements(replacements) {}
					clang::FrontendAction *create() override { return new ReplaceRefs(renames, replacements); }
			};
			return new Factory(renames, replacements);
	}

	virtual Impl* CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef) override {
		return new Impl(renames, replacements, ci.getSourceManager());
	}

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources, const Renames & renames) {
		clang::tooling::RefactoringTool tool(db, sources);
		auto factory = newFactory(renames, tool.getReplacements());
		if(save)
			return tool.runAndSave(factory);
		else {
			int ret = tool.run(factory);
			std::cout << "\nPatch shown below:" << std::endl;
			for(auto & replacement : tool.getReplacements()) std::cout << "  " << replacement.toString() << std::endl;
			return ret;
		}
	}

};


#endif // REPLACEREFS_H
