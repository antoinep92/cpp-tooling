#include <iostream>
#include <functional>

#include "llvm/Support/CommandLine.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "simple-parse.h"

static llvm::cl::OptionCategory ActionSelectorOptions("Action selector options");
static llvm::cl::extrahelp CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nThese options are for selecting the action on the code");
static llvm::cl::opt<bool> ActionParse("action-parse", llvm::cl::desc("parse C++ code, print classes and functions and what they use"));

int main(int argc, const char **argv) {
	clang::tooling::CommonOptionsParser OptionsParser(argc, argv);
	clang::tooling::ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

	std::function<int()> action;
	if(ActionParse) action = [&](){ return Tool.run(clang::tooling::newFrontendActionFactory<SimpleParseAction>()); };

	if(!action) {
		std::cerr << "no known action selected" << std::endl;
		return 1;
	}
	return action();
}

