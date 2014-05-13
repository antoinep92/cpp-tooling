#ifndef TOOLS_H
#define TOOLS_H
#include "common.h"
namespace tools {
	int find_demo(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources);
	int print_uses(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources);
	int replace_decls(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources);
	int replace_refs(const clang::tooling::CompilationDatabase & db, llvm::ArrayRef<std::string> sources, const std::unordered_map<std::string, std::string> & renames);

}
#endif // TOOLS_H
