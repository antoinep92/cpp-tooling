TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp test.cpp

QMAKE_CXX = clang++
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

HEADERS += \
    simple-parse.h \
    naming-convention.h
