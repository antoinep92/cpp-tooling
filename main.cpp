/*
 * Copyright (c) 2014
 *
 * C++ tools, see README for more info
 *
 * example usage:
 *		clang-parse -action-parse test.cpp -- -I/usr/lib/gcc/x86_64-unknown-linux-gnu/4.8.2/include -std=c++11
 *
 */
#include "tools.h"
static llvm::cl::OptionCategory my_options("Action selector options");
static llvm::cl::extrahelp common_help(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp extra_help("Help Message TODO\n\n");


// developped here
static llvm::cl::opt<bool>
print_uses("print-uses",			llvm::cl::desc("parse + print classes/functions declarations and what they use"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
find_demo(		"find-demo",		llvm::cl::desc("parse + finds an 'x' field inside classes inheriting from 'base'"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
replace_decls(	"replace-decls",	llvm::cl::desc("find all declarations and add a suffix"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
replace_refs(	"replace-refs",		llvm::cl::desc("rename a declaration and all its references"), llvm::cl::cat(my_options));


// from clang
static llvm::cl::opt<bool>
print_lex(		"print-lex",			llvm::cl::desc("lex + print tokens before pre-processor"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
print_preproc(	"print-preprocess",		llvm::cl::desc("lex + print tokens after pre-processor"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
print_macros(	"print-macros",			llvm::cl::desc("print #defines"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
print_decls(	"print-declarations",	llvm::cl::desc("parse + print declarations"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
print_ast(		"print-ast",			llvm::cl::desc("parse + print the whole AST"), llvm::cl::cat(my_options));
static llvm::cl::opt<bool>
print_source(	"print-source",			llvm::cl::desc("parse + resynthetize source"), llvm::cl::cat(my_options));


template<class Action> int runBuiltin(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources) {
	clang::tooling::ClangTool tool(db, sources);
	return tool.run(clang::tooling::newFrontendActionFactory<Action>());
}

int main(int argc, const char **argv) {
	clang::tooling::CommonOptionsParser option_parser(argc, argv);
	auto & db = option_parser.getCompilations();
	auto sources = option_parser.getSourcePathList();

	std::function<int()> action;
	if(print_uses)
		action = [&](){ return tools::print_uses(db, sources); };
	if(find_demo)
		action = [&](){ return tools::find_demo(db, sources); };
	if(replace_decls)
		action = [&](){ return tools::replace_decls(db, sources); };
	if(replace_refs)
		action = [&](){ return tools::replace_refs(db, sources, {{"test::array_counter", "array_cnt"},{"test::index_counter", "index_cnt"}}); };


	if(print_lex)
		action = [&](){ return runBuiltin<clang::DumpRawTokensAction>(db, sources); };
	if(print_preproc)
		action = [&](){ return runBuiltin<clang::DumpTokensAction>(db, sources); };
	if(print_macros)
		action = [&](){ return runBuiltin<clang::PrintPreprocessedAction>(db, sources); };
	if(print_decls)
		action = [&](){ return runBuiltin<clang::ASTDeclListAction>(db, sources); };
	if(print_ast)
		action = [&](){ return runBuiltin<clang::ASTDumpAction>(db, sources); };
	if(print_source)
		action = [&](){ return runBuiltin<clang::ASTPrintAction>(db, sources); };

	if(!action) {
		std::cerr << "no known action selected" << std::endl;
		return 1;
	} else return action();
}

