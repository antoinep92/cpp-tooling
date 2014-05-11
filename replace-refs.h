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

struct ReplaceRefs : clang::ASTFrontendAction {

	typedef std::unordered_map<std::string /* qualified original name */, std::string /* unqualified target name */> Renames;
	Renames renames;
	clang::tooling::Replacements & replacements;
	ReplaceRefs(const Renames & renames, clang::tooling::Replacements & replacements) :
		renames(renames), replacements(replacements) {}

	struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {

		struct Item {
			std::pair<std::string, std::string> rename;
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

		template<class X> void addReplacement(const X & xpr, const std::string & replacement) {
			auto start = sourceManager.getSpellingLoc(xpr.getLocStart()), end = sourceManager.getSpellingLoc(xpr.getLocEnd());
			std::string original(
				sourceManager.getCharacterData(start),
				sourceManager.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(end, 0, sourceManager, clang::LangOptions())).second
				- sourceManager.getDecomposedLoc(start).second
			);
			std::cout << '\t' << original << "  ==>  " << replacement << std::endl;
			replacements.insert(clang::tooling::Replacement(sourceManager, clang::CharSourceRange::getTokenRange(xpr.getSourceRange()), replacement));
		}
		void addReplacements() {
			std::cout << "computing patch..." << std::endl;
			for(auto & ip : index) {
				for(auto decl : ip.second.declarations) addReplacement(*decl, ip.second.rename.second);
				for(auto ref : ip.second.references) addReplacement(*ref, ip.second.rename.second);
			}

		}

		void dump() const {
			for(auto & ip : index) {
				std::cout << ip.second.rename.first << " ==> " << ip.second.rename.second << std::endl;
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
		int ret = tool.run(newFactory(renames, tool.getReplacements()));
		std::cout << "\nPatch shown below:" << std::endl;
		for(auto & replacement : tool.getReplacements()) {
			std::cout << "  " << replacement.toString() << std::endl;
		}
		return ret;
	}

};


#endif // REPLACEREFS_H
