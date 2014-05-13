TEMPLATE = app
CONFIG += console precompile_header
CONFIG -= app_bundle qt

SOURCES += main.cpp test.cpp print-uses.cpp find-demo.cpp replace-decls.cpp replace-refs.cpp

QMAKE_CXX = clang++
QMAKE_LINK = clang++

QMAKE_CXXFLAGS += -std=c++11 -D__STDC_LIMIT_MACROS=100 -D__STDC_CONSTANT_MACROS=100

LIBS += -lLLVM-3.4 \
        -lclang \
        -lclangTooling \
        -lclangDriver \
        -lclangASTMatchers \
        -lclangFrontend \
        -lclangSerialization \
        -lclangBasic \
        -lclangRewriteCore \
        -lclangParse \
        -lclangBasic \
        -lclangSema \
        -lclangAnalysis \
        -lclangAST \
        -lclangEdit \
        -lclangLex \

PRECOMPILED_HEADER = common.h

HEADERS += tools.h
