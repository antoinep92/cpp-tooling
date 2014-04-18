#ifndef PRINTUSES_H
#define PRINTUSES_H

#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

#include <iostream>


struct PrintUses : clang::ASTFrontendAction {

	struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {
		typedef clang::RecursiveASTVisitor<Impl> Parent;

		clang::SourceManager & sourceManager;
		clang::StringRef sourceFile;
		bool indent;

		Impl(clang::SourceManager & sourceManager, clang::StringRef sourceFile):
			sourceManager(sourceManager), sourceFile(sourceFile), indent(false) {}

		virtual void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
			std::cout << "Parsing file \"" << sourceFile.str() << '"' << std::endl;
			TraverseDecl(context.getTranslationUnitDecl());
		}

		bool VisitStmt(clang::Stmt * stmt) { // from RecursiveASTVisitor
			if(llvm::dyn_cast<clang::DeclRefExpr>(stmt)) {
				if(sourceManager.getFilename(stmt->getLocStart()).equals(sourceFile)) {
					clang::ValueDecl* decl = static_cast<clang::DeclRefExpr*>(stmt)->getDecl();
					if(llvm::dyn_cast<clang::FunctionDecl>(decl)) {
						if(sourceManager.getFilename(decl->getLocation()).equals(sourceFile)) {
							std::cout << 'L' << sourceManager.getExpansionLineNumber(stmt->getLocStart()) << '\t';
							if(indent) std::cout << '\t';
							std::cout << "\tUse " << decl->getQualifiedNameAsString() << std::endl;
						}
					}
				}
			}
			return true;
		}

		bool VisitDecl(clang::Decl *decl) { // from RecursiveASTVisitor
			bool func = llvm::dyn_cast<clang::FunctionDecl>(decl);
			if(llvm::dyn_cast<clang::CXXRecordDecl>(decl) || (func && decl->hasBody())) {
				if(sourceManager.getFilename(decl->getLocation()).equals(sourceFile)) {
					std::cout << 'L' << sourceManager.getExpansionLineNumber(decl->getLocStart()) << '\t';
					indent = func && llvm::dyn_cast<clang::CXXMethodDecl>(decl);
					if(indent) std::cout << '\t';
					if(func) std::cout << "function ";
					else std::cout << "class ";
					std::cout << (static_cast<clang::NamedDecl*>(decl))->getQualifiedNameAsString() << std::endl;
				}
			}
			return true;
		}

	};
	virtual Impl* CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef sourceFile) override {
		return new Impl(ci.getSourceManager(), sourceFile);
	}

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {
		clang::tooling::ClangTool tool(db, sources);
		return tool.run(clang::tooling::newFrontendActionFactory<PrintUses>());
	}
};

#endif // SIMPLEPARSE_H
