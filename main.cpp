#include <iostream>
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
	Replacements *Replace;
	//Callback(Replacements * Replace) : Replace(Replace) {}
	virtual void run(const MatchFinder::MatchResult & result) {
		if(const Decl * decl = result.Nodes.getNodeAs<Decl>("x"))
			decl->dump();
	}
};



int main(int argc, const char **argv) {
	CommonOptionsParser OptionsParser(argc, argv);
	ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	//return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>());


	ast_matchers::MatchFinder finder;
	Callback callback;
	finder.addMatcher(matcher, &callback);
	return Tool.run(newFrontendActionFactory(&finder));
}

