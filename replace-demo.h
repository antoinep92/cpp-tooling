#ifndef REPLACEDEMO_H
#define REPLACEDEMO_H

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
#include "clang/Lex/Lexer.h"

#include <sstream>
// reverse-enginered from http://llvm.org/svn/llvm-project/clang-tools-extra/trunk/remove-cstr-calls/RemoveCStrCalls.cpp

struct ReplaceDemo : clang::ast_matchers::MatchFinder {
	typedef clang::ast_matchers::MatchFinder Parent;

	struct Callback : clang::ast_matchers::MatchFinder::MatchCallback {
		clang::tooling::Replacements & replacements;
		Callback(clang::tooling::Replacements & replacements) : replacements(replacements) {}
		virtual void run(const MatchFinder::MatchResult & result) {
			if(const clang::Decl * decl = result.Nodes.getNodeAs<clang::Decl>("x")) {
				auto & sm = *result.SourceManager;
				auto start = sm.getSpellingLoc(decl->getLocStart()), end = sm.getSpellingLoc(decl->getLocEnd());
				std::string text(
							sm.getCharacterData(start),
							sm.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(end, 0, sm, clang::LangOptions())).second
							- sm.getDecomposedLoc(start).second
							);
				std::stringstream ss;
				ss << text << '_' << text.size();
				decl->dump();
				replacements.insert(clang::tooling::Replacement(sm, decl->getSourceRange(), ss.str()));
			}
		}
	};

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {

		using namespace clang::ast_matchers;
		auto match_decl = fieldDecl().bind("x");

		clang::tooling::RefactoringTool tool(db, sources);
		Callback callback(tool.getReplacements());
		ReplaceDemo replace_demo;
		replace_demo.addMatcher(match_decl, &callback);
		return tool.run(clang::tooling::newFrontendActionFactory(&replace_demo));
	}

};



#endif // REPLACEDEMO_H
