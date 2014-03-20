#include <iostream>
#include <vector>
#include <unordered_map>
using std::cout;
using std::endl;


#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Refactoring.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Frontend/CompilerInstance.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...");

namespace clang {
namespace ast_matchers {
AST_MATCHER_P(FieldDecl, inClass, internal::Matcher<CXXRecordDecl>, InnerMatcher) {
		const CXXRecordDecl *Parent = dynamic_cast<const CXXRecordDecl*>(Node.getParent());
		return (Parent && InnerMatcher.matches(*Parent, Finder, Builder));
}
}
}

auto matcher =
	fieldDecl(
		hasName("x"),
		isPublic(),
		inClass(
			isDerivedFrom("base")
		)
	).bind("x");


struct Callback : MatchFinder::MatchCallback {
	Replacements *replacements;
	//Callback(Replacements * replacements) : replacements(replacements) {}
	virtual void run(const MatchFinder::MatchResult & result) {
		if(const Decl * decl = result.Nodes.getNodeAs<Decl>("x")) {
			decl->dump();
			/*Replacement r(
				*result.SourceManager,
				CharSourceRange::getTokenRange(decl->getLocation()),
				"x_replaced"
			);
			cout << r.toString() << endl;
			Rewriter rw(*result.SourceManager, LangOptions());
			r.apply(rw);
			cout << std::string(rw.buffer_begin(), rw.buffer_end()) << endl;*/
		}
	}
};


struct MyAction : clang::ASTFrontendAction {

	struct Impl : clang::ASTConsumer, RecursiveASTVisitor<Impl> {
		typedef clang::RecursiveASTVisitor<Impl> Parent;
		using Parent::shouldVisitImplicitCode;
		using Parent::shouldVisitTemplateInstantiations;
		using Parent::shouldWalkTypesOfTypeLocs;
		using Parent::VisitDecl;
		using Parent::VisitType;

		SourceManager & sourceManager;
		StringRef sourceFile;

		bool indent;

		Impl(SourceManager & sourceManager, StringRef sourceFile) : sourceManager(sourceManager), sourceFile(sourceFile), indent(false) {}

		bool shouldVisitTemplateInstantiations() const { return false; }
		bool shouldVisitImplicitCode() const { return false; }
		bool shouldWalkTypesOfTypeLocs() const { return false; }

		virtual void HandleTranslationUnit(ASTContext & context) override { // from ASTConsumer
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

			bool func = dyn_cast<clang::FunctionDecl>(decl);
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


int main(int argc, const char **argv) {
	CommonOptionsParser optionsParser(argc, argv);
	ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());


	//return tool.run(newFrontendActionFactory<clang::DumpRawTokensAction>()); // print tokens before preprocess
	//return tool.run(newFrontendActionFactory<clang::DumpTokensAction>()); // print tokens after preprocess
	//return tool.run(newFrontendActionFactory<clang::PrintPreprocessedAction>()); // print all #defines
	//return tool.run(newFrontendActionFactory<clang::ASTDeclListAction>()); // print all declarations
	//return tool.run(newFrontendActionFactory<clang::ASTDumpAction>()); // print the whole AST
	//return tool.run(newFrontendActionFactory<clang::ASTPrintAction>()); // reformat the AST as source code

	return tool.run(newFrontendActionFactory<MyAction>());

	/*ast_matchers::MatchFinder finder;
	Callback callback;
	finder.addMatcher(matcher, &callback);
	return tool.run(newFrontendActionFactory(&finder));*/
}

