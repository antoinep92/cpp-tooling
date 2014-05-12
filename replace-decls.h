#ifndef REPLACEDECL_H
#define REPLACEDECL_H

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

// matches a field in a namespace
namespace clang { namespace ast_matchers {
	AST_MATCHER_P(Decl, inNamespace, internal::Matcher<clang::NamespaceDecl>, InnerMatcher) {
		auto ns = llvm::dyn_cast<const clang::NamespaceDecl>(Node.getDeclContext()->getEnclosingNamespaceContext());
			return (ns && InnerMatcher.matches(*ns, Finder, Builder));
	}
} }


// inspired by http://llvm.org/svn/llvm-project/clang-tools-extra/trunk/remove-cstr-calls/RemoveCStrCalls.cpp

struct ReplaceDecls : clang::ast_matchers::MatchFinder {

	struct Callback : clang::ast_matchers::MatchFinder::MatchCallback {
		clang::tooling::Replacements & replacements;
		Callback(clang::tooling::Replacements & replacements) : replacements(replacements) {}
		void run(const MatchFinder::MatchResult & result) override {
			if(const clang::Decl * decl = result.Nodes.getNodeAs<clang::Decl>("x")) {
				auto & sm = *result.SourceManager;
				auto start = sm.getSpellingLoc(decl->getLocStart()), end = sm.getSpellingLoc(decl->getLocEnd());
				std::string original(
							sm.getCharacterData(start),
							sm.getDecomposedLoc(clang::Lexer::getLocForEndOfToken(end, 0, sm, clang::LangOptions())).second
							- sm.getDecomposedLoc(start).second
							);
				std::stringstream ss;
				ss << original << '_' << original.size();
				std::string replacement = ss.str();
				//decl->dump();
				std::cout << original << "  ==>  " << replacement << std::endl;
				replacements.insert(clang::tooling::Replacement(sm, clang::CharSourceRange::getTokenRange(decl->getSourceRange()), replacement));
			}
		}
	};

	static int run(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {

		using namespace clang::ast_matchers;
		auto match_decl = fieldDecl(inNamespace(hasName("test"))).bind("x");

		clang::tooling::RefactoringTool tool(db, sources);
		Callback callback(tool.getReplacements());
		ReplaceDecls replace_decls;
		replace_decls.addMatcher(match_decl, &callback);
		auto ret = tool.run(clang::tooling::newFrontendActionFactory(&replace_decls));
		std::cout << "\nPatch shown below:" << std::endl;
		for(auto & replacement : tool.getReplacements()) {
			std::cout << "  " << replacement.toString() << std::endl;
		}
		return ret;
	}

};



#endif // REPLACEDECL_H
