/*
 * Copyright (c) 2014
 *
 * C++ tools, see README for more info
 *
 * example usage:
 *		clang-parse -action-parse test.cpp -- -I/usr/lib/gcc/x86_64-unknown-linux-gnu/4.8.2/include -std=c++11
 *
 */

#include <iostream>
#include <functional>

#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "simple-parse.h"

static llvm::cl::OptionCategory option_category("Action selector options");
static llvm::cl::extrahelp options_help(clang::tooling::CommonOptionsParser::HelpMessage);

// developped here
static llvm::cl::opt<bool>
print_use_graph("print-use-graph",		llvm::cl::desc("parse + print classes/functions declarations and what they use"));

// from clang
static llvm::cl::opt<bool>
print_lex(		"print-lex",			llvm::cl::desc("lex + print tokens before pre-processor"));
static llvm::cl::opt<bool>
print_preproc(	"print-preprocess",		llvm::cl::desc("lex + print tokens after pre-processor"));
static llvm::cl::opt<bool>
print_macros(	"print-macros",			llvm::cl::desc("print #defines"));
static llvm::cl::opt<bool>
print_decls(	"print-declarations",	llvm::cl::desc("parse + print declarations"));
static llvm::cl::opt<bool>
print_ast(		"print-ast",			llvm::cl::desc("parse + print the whole AST"));
static llvm::cl::opt<bool>
print_source(	"print-source",			llvm::cl::desc("parse + resynthetize source"));

int main(int argc, const char **argv) {
	clang::tooling::CommonOptionsParser option_parser(argc, argv);
	clang::tooling::ClangTool tool(option_parser.getCompilations(), option_parser.getSourcePathList());

	std::function<int()> action;
	if(print_use_graph)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<PrintUseGraph>()); };
	if(print_lex)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::DumpRawTokensAction>()); };
	if(print_preproc)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::DumpTokensAction>()); };
	if(print_macors)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::PrintPreprocessedAction>()); };
	if(print_decls)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::ASTDeclListAction>()); };
	if(print_ast)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::ASTDumpAction>()); };
	if(print_source)
		action = [&](){ return tool.run(clang::tooling::newFrontendActionFactory<clang::ASTPrintAction>()); };

	if(!action) {
		std::cerr << "no known action selected" << std::endl;
		return 1;
	} else return action();
}

