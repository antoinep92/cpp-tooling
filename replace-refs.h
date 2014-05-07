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

	typedef std::unordered_map<
		std::string,							/* original name */
		std::pair<
			clang::Decl*						/* canonical decl */,
			std::unordered_set<clang::Decl*>	/* all decls (including canonical) */
		>
	> NamedDecls;
	NamedDecls decls;							// built during first pass

	typedef std::unordered_map<
		clang::Decl*,								/* canonical decl */
		std::pair<
			std::unordered_set<clang::Decl*>,		/* all decls (first pass) */
			std::unordered_set<clang::DeclRefExpr*> /* all refs (second pass) */
		>
	> Refs;
	Refs refs;

	//clang::SourceManager & sourceManager;
	//clang::tooling::Replacements & replacements;

	void declsToRefs() {
		for(auto & d : decls) refs[d.second.first].first = d.second.second;
	}


	void processDecl(clang::NamedDecl * decl) { // first pass handler
		auto iter = renames.find(decl->getQualifiedNameAsString());
		if(iter != renames.end()) {
			auto & r = decls[iter->first];
			r.second.insert(decl);
			if(decl->isCanonicalDecl()) {
				assert(!r.first);
				r.first = decl;
			}
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
		auto iter = refs.find(ref->getDecl());
		if(iter != refs.end()) iter->second.second.insert(ref);
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

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {
		clang::tooling::ClangTool tool(db, sources);
		ReplaceRefs top;

		int r = tool.run(makeAction<ScanDecls>(top));
		//if(!r) return r;
		top.declsToRefs();
		r = tool.run(makeAction<ScanRefs>(top));
		return r;
		// TODO : build replacements from refs
	}
};


#endif // REPLACEREFS_H
