#ifndef SIMPLEPARSE_H
#define SIMPLEPARSE_H

#include "clang/Frontend/FrontendActions.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"

#include <iostream>


struct SimpleParseAction : clang::ASTFrontendAction {

	struct Impl : clang::ASTConsumer, clang::RecursiveASTVisitor<Impl> {
		typedef clang::RecursiveASTVisitor<Impl> Parent;
		using Parent::shouldVisitImplicitCode;
		using Parent::shouldVisitTemplateInstantiations;
		using Parent::shouldWalkTypesOfTypeLocs;
		using Parent::VisitDecl;
		using Parent::VisitType;

		clang::SourceManager & sourceManager;
		clang::StringRef sourceFile;

		bool indent;

		Impl(clang::SourceManager & sourceManager, clang::StringRef sourceFile):
			sourceManager(sourceManager), sourceFile(sourceFile), indent(false) {}

		bool shouldVisitTemplateInstantiations() const { return false; }
		bool shouldVisitImplicitCode() const { return false; }
		bool shouldWalkTypesOfTypeLocs() const { return false; }

		virtual void HandleTranslationUnit(clang::ASTContext & context) override { // from ASTConsumer
			std::cout << "Parsing file \"" << sourceFile.str() << '"' << std::endl;
			TraverseDecl(context.getTranslationUnitDecl());
			//context.getTranslationUnitDecl()->dump();
		}

		bool VisitStmt(clang::Stmt * stmt) {
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

		bool VisitDecl(clang::Decl *decl) {

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
		//bool VisitType(Type *type) { type->dump(); return false; }

	};
	virtual Impl* CreateASTConsumer(clang::CompilerInstance& ci, llvm::StringRef sourceFile) override {
		return new Impl(ci.getSourceManager(), sourceFile);
	}
};

#endif // SIMPLEPARSE_H
