#ifndef FINDDEMO_H
#define FINDDEMO_H

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

// matches a field in a specific class
namespace clang { namespace ast_matchers {
	AST_MATCHER_P(FieldDecl, inClass, internal::Matcher<CXXRecordDecl>, InnerMatcher) {
			const CXXRecordDecl * Parent = dynamic_cast<const CXXRecordDecl*>(Node.getParent());
			return (Parent && InnerMatcher.matches(*Parent, Finder, Builder));
	}
} }


struct FindDemo : clang::ast_matchers::MatchFinder {
	typedef clang::ast_matchers::MatchFinder Parent;

	struct Callback : clang::ast_matchers::MatchFinder::MatchCallback {
		void run(const MatchFinder::MatchResult & result) override {
			if(const clang::Decl * decl = result.Nodes.getNodeAs<clang::Decl>("x")) {
				decl->dump();
			}
		}
	};

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {

		using namespace clang::ast_matchers;
		auto match_x_field =
			fieldDecl(
				hasName("x"),
				isPublic(),
				inClass(
					isDerivedFrom("base")
				)
			).bind("x");
		clang::tooling::ClangTool tool(db, sources);
		Callback callback;
		FindDemo find_demo;
		find_demo.addMatcher(match_x_field, &callback);
		return tool.run(clang::tooling::newFrontendActionFactory(&find_demo));
	}

};


#endif // NAMINGCONVENTION_H
