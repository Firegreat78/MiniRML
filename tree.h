// Tree.h
#ifndef TREE_H
#define TREE_H

#include "SemNode.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

class Tree {
public:
    SemNode* n;
    Tree* Up;
    Tree* Left;
    Tree* Right;

    static Tree* Root;
    static Tree* Cur;

    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    void setLeft(SemNode* Data);
    void setRight(SemNode* Data);

    Tree* findUp(Tree* From, const std::string& id);
    Tree* findUpOneLevel(Tree* From, const std::string& id);

    // Семантические операции
    Tree* semInclude(const std::string& a, DATA_TYPE t, int line, int col);
    Tree* semIncludeConstant(const std::string& a, DATA_TYPE t, const std::string& value, int line, int col);
    void semSetParam(Tree* Addr, int n);
    void semSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types);
    void semControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col);
    Tree* semGetVar(const std::string& a, int line, int col);
    Tree* semGetFunct(const std::string& a, int line, int col);
    bool dupControl(Tree* Addr, const std::string& a);
    Tree* semEnterBlock(int line, int col);
    void semExitBlock();

    static void setCur(Tree* a) { Cur = a; }
    static Tree* getCur() { return Cur; }

    void print();
    static void semError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);
    static void interpError(const std::string& msg, const std::string& id = "", int line = -1, int col = -1);

    // Статические методы для интерпретации - исправленные сигнатуры
    static void setVarValue(const std::string& name, const SemNode& value, int line, int col);
    static SemNode getVarValue(const std::string& name, int line, int col);
    static SemNode executeArithmeticOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static SemNode executeShiftOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static SemNode executeComparisonOp(const SemNode& left, const SemNode& right, const std::string& op, int line, int col);
    static DATA_TYPE getMaxType(DATA_TYPE t1, DATA_TYPE t2);
    static SemNode castToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning = false);
    static bool canImplicitCast(DATA_TYPE from, DATA_TYPE to);
    static void executeFunctionCall(const std::string& funcName, const std::vector<SemNode>& args, int line, int col);

    // Методы для управления интерпретацией
    static void enableInterpretation();
    static void disableInterpretation();
    static bool isInterpretationEnabled();

    static void enableDebug();
    static void disableDebug();
    static bool isDebugEnabled();

    // Методы для вывода
    static void printDebugInfo(const std::string& message, int line = 0, int col = 0);
    static void printAssignment(const std::string& varName, const SemNode& value, int line, int col);
    static void printFunctionCall(const std::string& funcName, const std::vector<SemNode>& args, int line, int col);
    static void printArithmeticOp(const std::string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col);
    static void printTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const std::string& context, const std::string& expression, int line, int col);

    // Текущая функция для контекста
    static Tree* currentFunction;

    // Методы для управления текущей функцией
    static void setCurrentFunction(Tree* func) { currentFunction = func; }
    static Tree* getCurrentFunction() { return currentFunction; }

    // Создаёт глубокую копию поддерева (узел + его дочерние узлы)
    static Tree* cloneSubtree(Tree* node);

    static Tree* cloneRecursive(const Tree* node);
    static void fixUpPointers(Tree* copy, Tree* parentUp);

    static void enterFunctionCall(const std::string& funcName, int line = -1, int col = -1);
    static void exitFunctionCall();

    static void reset(); // сброс глобального состояния

private:
    void print(int depth);
    std::string makeLabel(const Tree* tree) const;

    // Флаг интерпретации
    static bool interpretationEnabled;
    static bool debug; // флаг для подробного вывода

    // глубина рекурсии
    static int recursionDepth;
    static const int MAX_RECURSION_DEPTH = 50;
};

#endif // TREE_H
