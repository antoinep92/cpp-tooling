#ifndef NAMINGCONVENTION_H
#define NAMINGCONVENTION_H


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

namespace clang { namespace ast_matchers {
	AST_MATCHER_P(FieldDecl, inClass, internal::Matcher<CXXRecordDecl>, InnerMatcher) {
			const CXXRecordDecl *Parent = dynamic_cast<const CXXRecordDecl*>(Node.getParent());
			return (Parent && InnerMatcher.matches(*Parent, Finder, Builder));
	}
} }

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






	//return tool.run(newFrontendActionFactory<clang::DumpRawTokensAction>()); // print tokens before preprocess
	//return tool.run(newFrontendActionFactory<clang::DumpTokensAction>()); // print tokens after preprocess
	//return tool.run(newFrontendActionFactory<clang::PrintPreprocessedAction>()); // print all #defines
	//return tool.run(newFrontendActionFactory<clang::ASTDeclListAction>()); // print all declarations
	//return tool.run(newFrontendActionFactory<clang::ASTDumpAction>()); // print the whole AST
	//return tool.run(newFrontendActionFactory<clang::ASTPrintAction>()); // reformat the AST as source code


	/*ast_matchers::MatchFinder finder;
	Callback callback;
	finder.addMatcher(matcher, &callback);
	return tool.run(newFrontendActionFactory(&finder));*/



#endif // NAMINGCONVENTION_H
