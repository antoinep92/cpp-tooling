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

struct ReplaceRefs {

	typedef std::unordered_map<
		std::string,	/* qualified original name */
		std::string		/* unqualified target name */
	> Renames;
	Renames renames;	// input

	struct Item {
		std::pair<std::string, std::string> rename;
		std::unordered_set<clang::Decl*> declarations;
		std::unordered_set<clang::DeclRefExpr*> references;
	};

	typedef std::unordered_map<
		clang::Decl*,	// canonical decl
		Item			// renaming info, declarations and references
	> Index;
	Index index;

	//clang::SourceManager & sourceManager;
	//clang::tooling::Replacements & replacements;

	ReplaceRefs(const Renames & renames) : renames(renames) {}

	void processDecl(clang::NamedDecl * decl) { // first pass handler
		auto iter = renames.find(decl->getQualifiedNameAsString());
		if(iter != renames.end()) {
			auto & item = index[decl->getCanonicalDecl()];
			item.rename = *iter;
			item.declarations.insert(decl->getUnderlyingDecl());
		}
	}

	struct ScanDecls : clang::ASTFrontendAction { // first pass traversal
		ReplaceRefs & parent;
		ScanDecls(ReplaceRefs & parent) : parent(parent) {}

		struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {
			ReplaceRefs & parent;
			Impl(ReplaceRefs & parent): parent(parent) {}

			void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
				std::cout << "Scanning decls..." << std::endl;
				TraverseDecl(context.getTranslationUnitDecl());
			}
			bool VisitDecl(clang::Decl * decl) { // from RecursiveASTVisitor
				if(llvm::dyn_cast<clang::NamedDecl>(decl)) parent.processDecl(static_cast<clang::NamedDecl*>(decl));
				return true;
			}
		}; // ScanDecls::Impl

		virtual Impl* CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) override {
			return new Impl(parent);
		}
	}; // ScanDecls




	void processRef(clang::DeclRefExpr * ref) { // second pass handler
		//ref->dump();
		// TODO : find dump definition and find why it makes this work!
		auto iter = index.find(ref->getDecl()->getCanonicalDecl());
		if(iter != index.end()) iter->second.references.insert(ref);
	}

	struct ScanRefs : clang::ASTFrontendAction { // second pass traversal
		ReplaceRefs & parent;
		ScanRefs(ReplaceRefs & parent) : parent(parent) {}

		struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {
			ReplaceRefs & parent;
			Impl(ReplaceRefs & parent) : parent(parent) {}

			void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
				std::cout << "Scanning refs..." << std::endl;
				TraverseDecl(context.getTranslationUnitDecl());
			}

			bool VisitStmt(clang::Stmt * stmt) { // from RecursiveASTVisitor
				if(llvm::dyn_cast<clang::DeclRefExpr>(stmt)) parent.processRef(static_cast<clang::DeclRefExpr*>(stmt));
				return true;
			}
		}; // ScanRefs::Impl

		virtual Impl* CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) override {
			return new Impl(parent);
		}
	}; // ScanRefs

	template<class Action> static clang::tooling::FrontendActionFactory * makeAction(ReplaceRefs & parent) {
		struct Factory : clang::tooling::FrontendActionFactory {
			ReplaceRefs & parent;
			Factory(ReplaceRefs & parent) : parent(parent) {}
			clang::FrontendAction *create() override { return new Action(parent); }
		};
		return new Factory(parent);
	}

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources, const Renames & renames) {
		clang::tooling::ClangTool tool(db, sources);
		ReplaceRefs top(renames);

		int r = tool.run(makeAction<ScanDecls>(top));
		//if(!r) return r;
		r = tool.run(makeAction<ScanRefs>(top));
		top.dump();
		return r;
		// TODO : build replacements from refs
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
};


#endif // REPLACEREFS_H
