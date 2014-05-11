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
	ReplaceRefs(const Renames & renames) : renames(renames) {}

	struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {

		struct Item {
			std::pair<std::string, std::string> rename;
			std::unordered_set<clang::Decl*> declarations;
			std::unordered_set<clang::DeclRefExpr*> references;
		};
		std::unordered_map<clang::Decl*, Item> index;

		const Renames & renames;
		clang::SourceManager & sourceManager;
		//clang::tooling::Replacements & replacements;

		Impl(const Renames & renames, clang::SourceManager & sourceManager) : renames(renames), sourceManager(sourceManager) {}

		void processDecl(clang::NamedDecl * decl) {
			auto iter = renames.find(decl->getQualifiedNameAsString());
			if(iter != renames.end()) {
				decl->dump();
				decl->getUnderlyingDecl()->dump();
				decl->getCanonicalDecl()->dump();
				auto & item = index[decl->getCanonicalDecl()];
				item.rename = *iter;
				item.declarations.insert(decl);
			}
		}

		void processRef(clang::DeclRefExpr * ref) {
			// TODO : find dump definition and find why it makes this work!
			auto name = ref->getDecl()->getQualifiedNameAsString();
			for(auto & i : index) {
				if(name == i.second.rename.first) {
					ref->dump();
					ref->getDecl()->dump();
					ref->getDecl()->getUnderlyingDecl()->dump();
					ref->getDecl()->getCanonicalDecl()->dump();
				}
			}
			auto iter = index.find(ref->getDecl()->getCanonicalDecl());
			if(iter != index.end()) iter->second.references.insert(ref);
		}

		void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
			std::cout << "Scanning decls..." << std::endl;
			TraverseDecl(context.getTranslationUnitDecl());
			dump();
		}
		bool VisitDecl(clang::Decl * decl) { // from RecursiveASTVisitor
			if(llvm::dyn_cast<clang::NamedDecl>(decl)) processDecl(static_cast<clang::NamedDecl*>(decl));
			return true;
		}

		bool VisitStmt(clang::Stmt * stmt) { // from RecursiveASTVisitor
			if(llvm::dyn_cast<clang::DeclRefExpr>(stmt)) processRef(static_cast<clang::DeclRefExpr*>(stmt));
			return true;
		}

		void dump() const {
			for(auto & ip : index) {
				std::cout << ip.second.rename.first << " ==> " << ip.second.rename.second << std::endl;
				std::cout << '\t' << "cannonical declaration : " << ip.first << std::endl;
				std::cout << '\t' << "declarations:" << std::endl;
				for(auto & decl : ip.second.declarations) std::cout << "\t\t" << decl << std::endl;
				std::cout << '\t' << "referencing expressions:" << std::endl;
				for(auto & ref : ip.second.references) std::cout << "\t\t" << ref << std::endl;
			}
		}
	}; // Impl

	static clang::tooling::FrontendActionFactory * newFactory(const Renames & renames) {
			struct Factory : clang::tooling::FrontendActionFactory {
					const Renames & renames;
					Factory(const Renames & renames) : renames(renames) {}
					clang::FrontendAction *create() override { return new ReplaceRefs(renames); }
			};
			return new Factory(renames);
	}

	virtual Impl* CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef) override {
		return new Impl(renames, ci.getSourceManager());
	}

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources, const Renames & renames) {
		clang::tooling::ClangTool tool(db, sources);
		return tool.run(newFactory(renames));
		// TODO : build replacements from refs
	}

};


#endif // REPLACEREFS_H