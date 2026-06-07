#include "Diagram.h"
#include "tree.h"
#include <iostream>
#include <cmath>

#include <QThread>
#include <QException>

using namespace std;

class abrupt_parse_finish : public runtime_error
{
public:
    abrupt_parse_finish(const char* ch) : runtime_error(ch) {}
};

constexpr double PI = 3.1415926535897932384626433832795;
constexpr double DEG_TO_RAD_FACTOR = PI / 180.0;
constexpr double RAD_TO_DEG_FACTOR = 180.0 / PI;

constexpr double MIN_VAL = 1e-24;

inline double deg2rad(double degrees) {
    return degrees * DEG_TO_RAD_FACTOR;
}

inline double rad2deg(double radians) {
    return radians * RAD_TO_DEG_FACTOR;
}

// Конструктор
Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(LexemType::EMPTY), curLex(), currentDeclType(DATA_TYPE::TYPE_INT) {
    pushTok.clear();
    pushLex.clear();
}

vector<pair<string, SemNode*>> Diagram::getAllCurrentVariables()
{
    std::vector<std::pair<std::string, SemNode*>> result;
    result.reserve(currentVariables.size());
    for (const auto& [key, stack] : currentVariables)
    {
        if (!stack.empty()) result.emplace_back(key, stack.top());
    }
    return result;
}

void Diagram::synError(const string& msg)
{
    auto lc = sc->getLineCol();
    cerr << "Syntax error: " << msg;
    if (!curLex.empty()) cerr << " (near '" << curLex << "')";
    cerr << endl << "(line " << lc.first << ":" << lc.second << ")" << endl;

    QString const error_string = (curLex.empty() ?
                                QString("Syntax error: %1 (line %2:%3)")
                                    .arg(QString::fromStdString(msg))
                                    .arg(lc.first)
                                    .arg(lc.second) :
                                QString("Syntax error: %1 (near '%2') (line %3:%4)")
                                    .arg(QString::fromStdString(msg))
                                    .arg(curLex.c_str())
                                    .arg(lc.first)
                                    .arg(lc.second)
    );
    throw runtime_error(error_string.toStdString());
}

void Diagram::lexError()
{
    auto lc = sc->getLineCol();
    QString const error_string = QString("Lex error: invalid token '%1' (line %2:%3)")
                    .arg(curLex.c_str())
                    .arg(lc.first)
                    .arg(lc.second);
    cerr << error_string.constData();
    throw runtime_error(error_string.toStdString());
}

void Diagram::interpError(const string& msg)
{
    auto lc = sc->getLineCol();
    Tree::interpError(msg, curLex, lc.first, lc.second);
}

void Diagram::semError(const string& msg) {
    auto lc = sc->getLineCol();
    Tree::semError(msg, curLex, lc.first, lc.second);
}

LexemType Diagram::nextToken()
{
    if (!pushTok.empty())
    {
        LexemType t = pushTok.back();
        pushTok.pop_back();
        string lx = pushLex.back();
        pushLex.pop_back();
        curTok = t;
        curLex = lx;
        if (curTok == LexemType::T_ERR) lexError();
        return curTok;
    }

    string lex;
    curTok = sc->getNextLex(lex);
    curLex = lex;

    if (curTok == LexemType::T_ERR) lexError();
    return curTok;
}

LexemType Diagram::peekToken() {
    LexemType t = nextToken();
    pushBack(t, curLex);
    return t;
}

void Diagram::pushBack(LexemType tok, const string& lex) {
    pushTok.push_back(tok);
    pushLex.push_back(lex);
}

// Вспомогательные методы для интерпретации
void Diagram::pushValue(const SemNode& node) {
    evalStack.push(node);
}

SemNode Diagram::popValue() {
    if (evalStack.empty()) {
        semError("internal error: evalStack is empty");
    }
    SemNode node = evalStack.top();
    evalStack.pop();
    return node;
}

SemNode Diagram::evaluateLiteral(const string& value, DATA_TYPE type) {
    SemNode result;
    result.DataType = type;
    result.hasValue = true;

    try {
        if (type == DATA_TYPE::TYPE_BOOL) {
            result.Value.v_bool = (value == "true");
        }
        else if (type == DATA_TYPE::TYPE_DOUBLE)
        {
            // Обрабатываем отрицательные числа напрямую
            string valueToParse = value;
            bool isNegative = false;

            if (!valueToParse.empty() && valueToParse[0] == '-') {
                isNegative = true;
                valueToParse = valueToParse.substr(1);
            }

            double val = stod(valueToParse, nullptr);

            if (isNegative) {
                val = -val;
            }

            result.Value.v_double = val;
        }
        else
        {
            // Обрабатываем отрицательные числа напрямую
            string valueToParse = value;
            bool isNegative = false;

            if (!valueToParse.empty() && valueToParse[0] == '-') {
                isNegative = true;
                valueToParse = valueToParse.substr(1);
            }

            long long val = stoll(valueToParse, nullptr, 0);

            if (isNegative) {
                val = -val;
            }

            if (type == DATA_TYPE::TYPE_SHORT_INT) {
                result.Value.v_int16 = static_cast<int16_t>(val);
            }
            else if (type == DATA_TYPE::TYPE_INT) {
                result.Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (type == DATA_TYPE::TYPE_LONG_INT) {
                result.Value.v_int64 = static_cast<int64_t>(val);
            }
        }
    }
    catch (const exception& e) {
        semError("invalid literal format: " + string(e.what()));
    }

    return result;
}

void Diagram::executeAssignment(const string& varName, DATA_TYPE exprType, int line, int col) {
    SemNode value = popValue();
    Tree* varNode = Tree::Cur->semGetVar(varName, line, col);
    DATA_TYPE varType = varNode->n->DataType;

    bool varIsInt = (varType == DATA_TYPE::TYPE_INT || varType == DATA_TYPE::TYPE_SHORT_INT || varType == DATA_TYPE::TYPE_LONG_INT);
    bool exprIsInt = (exprType == DATA_TYPE::TYPE_INT || exprType == DATA_TYPE::TYPE_SHORT_INT || exprType == DATA_TYPE::TYPE_LONG_INT);

    if (!(
            (varIsInt && exprIsInt) ||
            (varType == DATA_TYPE::TYPE_BOOL && exprType == DATA_TYPE::TYPE_BOOL) ||
            (varType == DATA_TYPE::TYPE_DOUBLE && exprType == DATA_TYPE::TYPE_DOUBLE)
            ))
    {
        semError("type mismatch in assignment for '" + varName + "'");
    }

    Tree::setVarValue(varName, value, line, col);

    if (!Tree::isInterpretationEnabled()) return;

    auto formatAssign = [&](const auto& val) {
        return QString("[%1:%2] Executed assignment '%3 = %4'")
        .arg(line)
            .arg(col)
            .arg(QString::fromStdString(varName))
            .arg(val);
    };

    QString s;

    switch (varType)
    {
    case DATA_TYPE::TYPE_SHORT_INT: s = formatAssign(varNode->n->Value.v_int16); break;
    case DATA_TYPE::TYPE_INT:       s = formatAssign(varNode->n->Value.v_int32); break;
    case DATA_TYPE::TYPE_LONG_INT:  s = formatAssign(varNode->n->Value.v_int64); break;
    case DATA_TYPE::TYPE_BOOL:      s = formatAssign(varNode->n->Value.v_bool);  break;
    case DATA_TYPE::TYPE_DOUBLE:    s = formatAssign(varNode->n->Value.v_double); break;
    default: break;
    }

    waitForButtonPress(s);
}

// Точка входа
void Diagram::ParseProgram(bool isInterp, bool isDebug)
{
    SemNode* rootNode = new SemNode();
    rootNode->id = "<global scope>";
    rootNode->DataType = DATA_TYPE::TYPE_SCOPE;
    rootNode->line = 0;
    rootNode->col = 0;
    Tree* rootTree = new Tree(rootNode, nullptr);
    Tree::setCur(rootTree);
    scopeInitializedVarsAmountStack.push(0);

    if (isInterp) {
        Tree::enableInterpretation();
    }
    else {
        Tree::disableInterpretation();
    }

    if (isDebug) {
        Tree::enableDebug();
    }
    else {
        Tree::disableDebug();
    }

    Program();

    LexemType t = nextToken();
    if (t != LexemType::T_END) synError("there's a text at the end of the program");

    if (!isInterp) {
        rootTree->print();
    }
}

// Program -> (TopDecl)*
void Diagram::Program()
{
    LexemType t = peekToken();
    while (t == LexemType::KW_VOID ||
           t == LexemType::KW_INT ||
           t == LexemType::KW_SHORT ||
           t == LexemType::KW_LONG ||
           t == LexemType::KW_DOUBLE ||
           t == LexemType::KW_BOOL) {
        TopDecl();
        t = peekToken();
    }
}

// TopDecl -> Function | VarDecl
void Diagram::TopDecl() {
    LexemType t = peekToken();
    if (t == LexemType::KW_VOID) {
        Function();
    }
    else {
        VarDecl();
    }
}

// Function -> 'void' IDENT '(' ParamListOpt ')' Block
void Diagram::Function() {
    LexemType t = nextToken();
    if (t != LexemType::KW_VOID) synError("expected 'void' in function declaration");

    t = nextToken();
    if (t != LexemType::IDENT && t != LexemType::KW_MAIN) synError("expected function name (IDENT)");
    string funcName = curLex;
    auto pos = sc->getLineCol();

    Tree* funcNode = Tree::Cur->semInclude(funcName, DATA_TYPE::TYPE_FUNCT, pos.first, pos.second);
    Tree* savedCur = Tree::Cur;

    if (funcNode && funcNode->Left) {
        Tree::setCur(funcNode->Left);
    }
    else {
        Tree::Cur->semEnterBlock(pos.first, pos.second);
    }

    t = nextToken();
    if (t != LexemType::LPAREN) synError("expected '(' after function name");

    vector<DATA_TYPE> paramTypes;
    int paramCount = 0;
    t = peekToken();
    if (t != LexemType::RPAREN)
    {
        for (;;)
        {
            t = nextToken();
            if (!(
                    t == LexemType::KW_INT ||
                    t == LexemType::KW_SHORT ||
                    t == LexemType::KW_LONG ||
                    t == LexemType::KW_BOOL ||
                    t == LexemType::KW_DOUBLE
                    ))
                synError("expected parameter type");

            DATA_TYPE ptype = DATA_TYPE::TYPE_INT;
            if (t == LexemType::KW_SHORT) ptype = DATA_TYPE::TYPE_SHORT_INT;
            else if (t == LexemType::KW_LONG) ptype = DATA_TYPE::TYPE_LONG_INT;
            else if (t == LexemType::KW_BOOL) ptype = DATA_TYPE::TYPE_BOOL;
            else if (t == LexemType::KW_DOUBLE) ptype = DATA_TYPE::TYPE_DOUBLE;
            t = nextToken();
            if (t != LexemType::IDENT) synError("expected parameter name");
            string paramName = curLex;
            auto ppos = sc->getLineCol();

            Tree* paramNode = Tree::Cur->semInclude(paramName, ptype, ppos.first, ppos.second);
            if (paramNode && paramNode->n) {
                paramNode->n->hasValue = true; // Mark parameter as initialized
            }
            paramTypes.push_back(ptype);
            paramCount++;

            t = peekToken();
            if (t == LexemType::COMMA) {
                nextToken();
                continue;
            }
            else {
                break;
            }
        }
    }

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after parameter list");

    funcNode->semSetParam(funcNode, paramCount);
    funcNode->semSetParamTypes(funcNode, paramTypes);

    // Set current function
    Tree::setCurrentFunction(funcNode);

    // Control interpretation flag: disable for all functions except main
    bool wasInterpretationEnabled = Tree::isInterpretationEnabled();
    if (funcName != "main" || paramCount != 0) {
        Tree::disableInterpretation();
    }

    if (funcNode && funcNode->n) {
        funcNode->n->bodyStartPos = sc->getPos();
    }

    // Function body
    Block();

    if (funcNode && funcNode->n) {
        funcNode->n->bodyEndPos = sc->getPos();
    }

    // Reset current function
    Tree::setCurrentFunction(nullptr);

    // Restore previous interpretation flag state
    if (funcName != "main" || paramCount != 0) {
        if (wasInterpretationEnabled) {
            Tree::enableInterpretation();
        }
        else {
            Tree::disableInterpretation();
        }
    }

    Tree::setCur(savedCur);
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    LexemType t = nextToken();
    if (!(t == LexemType::KW_INT ||
          t == LexemType::KW_SHORT ||
          t == LexemType::KW_LONG ||
          t == LexemType::KW_BOOL ||
          t == LexemType::KW_DOUBLE
          ))
        synError("expected type in variable declaration");

    if (t == LexemType::KW_SHORT) currentDeclType = DATA_TYPE::TYPE_SHORT_INT;
    else if (t == LexemType::KW_LONG) currentDeclType = DATA_TYPE::TYPE_LONG_INT;
    else if (t == LexemType::KW_BOOL) currentDeclType = DATA_TYPE::TYPE_BOOL;
    else if (t == LexemType::KW_DOUBLE) currentDeclType = DATA_TYPE::TYPE_DOUBLE;
    else currentDeclType = DATA_TYPE::TYPE_INT;

    IdInitList();

    t = nextToken();
    if (t != LexemType::SEMI) synError("expected ';' at end of variable declaration");
}

// IdInitList -> IdInit (',' IdInit)*
void Diagram::IdInitList() {
    IdInit();
    LexemType t = peekToken();
    while (t == LexemType::COMMA) {
        nextToken();
        IdInit();
        t = peekToken();
    }
}

// IdInit -> IDENT [ = Expr ]
void Diagram::IdInit() {
    LexemType t = nextToken();
    if (t != LexemType::IDENT) synError("expected identifier in declaration list");
    string varName = curLex;
    auto pos = sc->getLineCol();

    Tree* varNode = Tree::Cur->semInclude(varName, currentDeclType, pos.first, pos.second);

    t = peekToken();
    if (t == LexemType::ASSIGN)
    {
        nextToken();

        DATA_TYPE exprType = Expr();
        SemNode value = popValue();

        DATA_TYPE varType = varNode->n->DataType;
        bool varIsInt = (varType == DATA_TYPE::TYPE_INT || varType == DATA_TYPE::TYPE_SHORT_INT || varType == DATA_TYPE::TYPE_LONG_INT);
        bool exprIsInt = (exprType == DATA_TYPE::TYPE_INT || exprType == DATA_TYPE::TYPE_SHORT_INT || exprType == DATA_TYPE::TYPE_LONG_INT);

        if (!(
                (varIsInt && exprIsInt) ||
                (varType == DATA_TYPE::TYPE_BOOL && exprType == DATA_TYPE::TYPE_BOOL) ||
                (varType == DATA_TYPE::TYPE_DOUBLE && exprType == DATA_TYPE::TYPE_DOUBLE)
                ))
            semError("type mismatch in initialization of variable '" + varName + "'");

        Tree::setVarValue(varName, value, pos.first, pos.second);
    }

    if (!Tree::isInterpretationEnabled()) return;

    currentVariables[varName].push(varNode->n);
    scopeAddedVariablesStack.push(varName);
    scopeInitializedVarsAmountStack.top()++;

    auto formatVar = [&](const auto& value, const QString& typeStr) {
        return QString("[%1:%2] Declared variable of type %3 '%4 = %5'")
        .arg(pos.first)
            .arg(pos.second)
            .arg(typeStr)
            .arg(QString::fromStdString(varName))
            .arg(value);
    };

    QString s;

    if (!varNode->n->hasValue) {
        QString typeStr;
        switch (varNode->n->DataType) {
        case DATA_TYPE::TYPE_SHORT_INT: typeStr = "short"; break;
        case DATA_TYPE::TYPE_INT:       typeStr = "int"; break;
        case DATA_TYPE::TYPE_LONG_INT:  typeStr = "long"; break;
        case DATA_TYPE::TYPE_BOOL:      typeStr = "bool"; break;
        case DATA_TYPE::TYPE_DOUBLE:    typeStr = "double"; break;
        default: typeStr = "unknown"; break;
        }
        s = QString("[%1:%2] Declared variable of type %3 '%4 = [uninitialized]'")
                .arg(pos.first)
                .arg(pos.second)
                .arg(typeStr)
                .arg(QString::fromStdString(varName));
    } else switch (varNode->n->DataType) {
        case DATA_TYPE::TYPE_SHORT_INT: s = formatVar(varNode->n->Value.v_int16, "short"); break;
        case DATA_TYPE::TYPE_INT:       s = formatVar(varNode->n->Value.v_int32, "int"); break;
        case DATA_TYPE::TYPE_LONG_INT:  s = formatVar(varNode->n->Value.v_int64, "long"); break;
        case DATA_TYPE::TYPE_BOOL:      s = formatVar(varNode->n->Value.v_bool, "bool"); break;
        case DATA_TYPE::TYPE_DOUBLE:    s = formatVar(varNode->n->Value.v_double, "double"); break;
        default: break;
        }

    waitForButtonPress(s);
}

// Block -> '{' BlockItems '}'
void Diagram::Block()
{
    LexemType t = nextToken();
    if (t != LexemType::LBRACE) synError("expected '{' to start block");

    // position after '{' — beginning of block body
    size_t posAfterLBrace = sc->getPos();

    auto lc = sc->getLineCol();
    Tree::Cur->semEnterBlock(lc.first, lc.second);

    // count the amount of unique (no dups anyway) initialized vars
    if (Tree::isInterpretationEnabled())
        scopeInitializedVarsAmountStack.push(0);

    // If we are parsing a function body (currentFunction is set), save the body start correctly
    Tree* curFunc = Tree::getCurrentFunction();
    if (curFunc && curFunc->n) {
        // write only if not previously written
        if (curFunc->n->bodyStartPos == 0) curFunc->n->bodyStartPos = posAfterLBrace;
    }

    BlockItems();

    t = nextToken();
    if (t != LexemType::RBRACE) synError("expected '}' to end block");

    // correct body end position (after '}')
    if (curFunc && curFunc->n) {
        curFunc->n->bodyEndPos = sc->getPos();
    }

    Tree::Cur->semExitBlock();
    if (Tree::isInterpretationEnabled())
    {
        for (int i = 0; i < scopeInitializedVarsAmountStack.top(); i++)
        {
            std::string const& varName = scopeAddedVariablesStack.top();
            currentVariables[varName].pop();
            scopeAddedVariablesStack.pop();
        }
        scopeInitializedVarsAmountStack.pop();
    }
}

// BlockItems -> ( VarDecl | Stmt )*
void Diagram::BlockItems() {
    LexemType t = peekToken();
    while (t != LexemType::RBRACE && t != LexemType::T_END)
    {
        if (t == LexemType::KW_INT ||
            t == LexemType::KW_SHORT ||
            t == LexemType::KW_LONG ||
            t == LexemType::KW_BOOL ||
            t == LexemType::KW_DOUBLE)
        {
            VarDecl();
        }
        else
        {
            Stmt();
        }
        t = peekToken();
    }
}

// Stmt -> ';' | Block | Assign | SwitchStmt | CallStmt | WhileStmt | MoveStmt | RotateStmt
void Diagram::Stmt() {
    LexemType t = peekToken();

    if (t == LexemType::SEMI)
    {
        nextToken(); // empty statement
        return;
    }

    if (t == LexemType::LBRACE)
    {
        Block();
        return;
    }

    if (t == LexemType::KW_SWITCH)
    {
        SwitchStmt();
        return;
    }

    if (t == LexemType::IDENT)
    {
        LexemType tokIdent = nextToken();
        string savedName = curLex;
        LexemType t2 = peekToken();

        pushBack(tokIdent, savedName);

        if (t2 == LexemType::ASSIGN)
        {
            Assign();
            t = nextToken();
            if (t != LexemType::SEMI) synError("expected ';' after assignment statement");
            return;
        }
        else if (t2 == LexemType::LPAREN)
        {
            CallStmt();
            return;
        }
        else
        {
            synError("expected '=' (assignment) or '(' (function call) after identifier");
        }
    }

    if (t == LexemType::KW_IF)
    {
        IfStmt();
        return;
    }

    if (t == LexemType::KW_WHILE)
    {
        WhileStmt();
        return;
    }

    if (t == LexemType::KW_MOVE)
    {
        MoveStmt();
        return;
    }

    if (t == LexemType::KW_ROTATE)
    {
        RotateStmt();
        return;
    }

    synError("unknown statement form");
}

// CallStmt -> Call ';'
void Diagram::CallStmt()
{
    Call();

    LexemType t = nextToken();
    if (t != LexemType::SEMI) synError("expected ';' after function call");
}

// Assign -> IDENT = Expr
void Diagram::Assign() {
    LexemType t = nextToken();
    if (t != LexemType::IDENT) synError("expected identifier in assignment");
    string name = curLex;
    auto lc = sc->getLineCol();

    Tree* leftNode = Tree::Cur->semGetVar(name, lc.first, lc.second);
    if (leftNode->n->DataType == DATA_TYPE::TYPE_FUNCT) {
        semError("cannot assign to function '" + name + "'");
    }

    t = nextToken();
    if (t != LexemType::ASSIGN) synError("expected '=' in assignment");

    DATA_TYPE rtype = Expr();
    DATA_TYPE ltype = leftNode->n->DataType;

    bool lIsInt = (ltype == DATA_TYPE::TYPE_INT || ltype == DATA_TYPE::TYPE_SHORT_INT || ltype == DATA_TYPE::TYPE_LONG_INT);
    bool rIsInt = (rtype == DATA_TYPE::TYPE_INT || rtype == DATA_TYPE::TYPE_SHORT_INT || rtype == DATA_TYPE::TYPE_LONG_INT);

    if (!(
            (lIsInt && rIsInt) ||
            (ltype == DATA_TYPE::TYPE_BOOL && rtype == DATA_TYPE::TYPE_BOOL) ||
            (ltype == DATA_TYPE::TYPE_DOUBLE && rtype == DATA_TYPE::TYPE_DOUBLE)
            ))
    {
        semError("type mismatch in assignment for '" + name + "'");
    }

    // Perform assignment
    executeAssignment(name, rtype, lc.first, lc.second);
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
void Diagram::SwitchStmt()
{
    LexemType t = nextToken();
    if (t != LexemType::KW_SWITCH) synError("expected 'switch'");

    t = nextToken();
    if (t != LexemType::LPAREN) synError("expected '(' after 'switch'");

    DATA_TYPE stype = Expr();
    if (!(stype == DATA_TYPE::TYPE_INT || stype == DATA_TYPE::TYPE_SHORT_INT || stype == DATA_TYPE::TYPE_LONG_INT)) {
        semError("switch expression type must be integer (int/short/long)");
    }

    SemNode sVal = popValue();
    if (!sVal.hasValue) interpError("switch expression is uninitialized");
    long long switchVal = 0;
    if (sVal.DataType == DATA_TYPE::TYPE_SHORT_INT) switchVal = sVal.Value.v_int16;
    else if (sVal.DataType == DATA_TYPE::TYPE_INT) switchVal = sVal.Value.v_int32;
    else if (sVal.DataType == DATA_TYPE::TYPE_LONG_INT) switchVal = sVal.Value.v_int64;

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after switch expression");

    t = nextToken();
    if (t != LexemType::LBRACE) synError("expected '{' for switch body");

    bool anyMatched = false;
    bool endSwitch = false;

    // Process ALL case branches, do not break the loop on first break,
    // but if endSwitch==true — parse further branches only semantically (do not execute).
    LexemType pk = peekToken();
    while (pk == LexemType::KW_CASE)
    {
        if (!endSwitch)
        {
            // CaseStmt returns true if a break was encountered in the executed branch
            if (CaseStmt(switchVal, anyMatched))
            {
                endSwitch = true;
            }
        }
        else {
            // already had a break — further parsing is semantic only: temporarily disable interpretation
            bool saveInterp = Tree::isInterpretationEnabled();
            if (saveInterp) Tree::disableInterpretation();
            CaseStmt(switchVal, anyMatched);
            if (saveInterp) Tree::enableInterpretation();
        }
        pk = peekToken();
    }

    if (peekToken() == LexemType::KW_DEFAULT)
    {
        if (!endSwitch) {
            if (DefaultStmt(anyMatched))
            {
                endSwitch = true;
            }
        }
        else
        {
            bool saveInterp = Tree::isInterpretationEnabled();
            if (saveInterp) Tree::disableInterpretation();
            DefaultStmt(anyMatched);
            if (saveInterp) Tree::enableInterpretation();
        }
    }

    t = nextToken();
    if (t != LexemType::RBRACE) synError("expected '}' at end of switch");
}

// CaseStmt -> 'case' Literal ':' Stmt*
bool Diagram::CaseStmt(long long switchVal, bool& anyMatched) {
    LexemType t = nextToken();
    if (t != LexemType::KW_CASE) synError("expected 'case'");

    t = nextToken();
    if (!(t == LexemType::DEC_LITERAL || t == LexemType::HEX_LITERAL))
    {
        semError("case accepts only a numeric literal");
    }

    long long caseVal = 0;
    try {
        caseVal = stoll(curLex, nullptr, 0);
    }
    catch (...) {
        semError("invalid literal in case");
    }

    t = nextToken();
    if (t != LexemType::COLON) synError("expected ':' after case value");

    bool matched = (!anyMatched && caseVal == switchVal);
    if (matched) anyMatched = true;

    bool branchBroken = false; // encountered break in this branch (executed or not)

    for (;;)
    {
        LexemType p = peekToken();
        if (p == LexemType::KW_CASE || p == LexemType::KW_DEFAULT || p == LexemType::RBRACE || p == LexemType::T_END) break;

        if (p == LexemType::KW_BREAK) {
            nextToken(); // consume KW_BREAK
            LexemType semi = nextToken();
            if (semi != LexemType::SEMI) synError("expected ';' after break");

            // regardless of matched — need to continue syntactic parsing
            // but mark that break occurred (if matched==true — this will stop execution of the outer switch)
            branchBroken = true;
            // after break all remaining statements in this branch are unreachable,
            // so we will parse them only semantically (not executing)
            continue;
        }

        // Normal statement parsing: execute only if matched and no break yet
        if (Tree::isInterpretationEnabled()) {
            if (matched && !branchBroken) {
                Stmt(); // execute
            }
            else {
                bool save = Tree::isInterpretationEnabled();
                if (save) Tree::disableInterpretation();
                Stmt(); // semantic only
                if (save) Tree::enableInterpretation();
            }
        }
        else {
            Stmt(); // interpretation disabled — semantic only
        }
    }

    // return true only if the matched branch included a break (then the outer Switch will stop execution)
    return (matched && branchBroken);
}

// DefaultStmt -> 'default' ':' Stmt*
bool Diagram::DefaultStmt(bool anyMatched) {
    LexemType t = nextToken();
    if (t != LexemType::KW_DEFAULT) synError("expected 'default'");

    t = nextToken();
    if (t != LexemType::COLON) synError("expected ':' after default");

    bool matched = !anyMatched;

    while (true) {
        LexemType p = peekToken();
        if (p == LexemType::KW_CASE || p == LexemType::RBRACE || p == LexemType::T_END) break;

        if (p == LexemType::KW_BREAK) {
            nextToken();
            LexemType semi = nextToken();
            if (semi != LexemType::SEMI) synError("expected ';' after break");
            return matched;
        }
        else
        {
            if (Tree::isInterpretationEnabled())
            {
                if (matched) Stmt();
                else
                {
                    bool saveInterp = Tree::isInterpretationEnabled();
                    if (saveInterp) Tree::disableInterpretation();
                    Stmt();
                    if (saveInterp) Tree::enableInterpretation();
                }
            }
            else
            {
                Stmt();
            }
        }
    }

    return false;
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call()
{
    LexemType t = nextToken();
    if (t != LexemType::IDENT) synError("expected function name in call");
    string fname = curLex;
    auto lc = sc->getLineCol();

    Tree* fnode = Tree::Cur->semGetFunct(fname, lc.first, lc.second);

    t = nextToken();
    if (t != LexemType::LPAREN) synError("expected '(' after function name");

    vector<SemNode> args;
    ArgListOpt(args);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after argument list");

    // Semantic check of parameter types
    vector<DATA_TYPE> argTypes;
    for (const auto& arg : args) argTypes.push_back(arg.DataType);
    fnode->semControlParamTypes(fnode, argTypes, lc.first, lc.second);

    // Log the call
    Tree::printFunctionCall(fname, args, lc.first, lc.second);

    // If interpretation is disabled — do not execute
    if (!Tree::isInterpretationEnabled()) return;

    // Check recursion limit (enter the call)
    Tree::enterFunctionCall(fname, lc.first, lc.second);

    // Get function scope — the original one (parsed during compilation)
    Tree* funcScope = fnode->Left;
    if (!funcScope) interpError("missing function body when calling '" + fname + "'");

    // Save parser / context state
    size_t savedPos = sc->getPos();
    auto savedPushTok = pushTok;
    auto savedPushLex = pushLex;
    LexemType savedCurTok = curTok;
    string savedCurLex = curLex;
    Tree* savedCurTree = Tree::getCur();
    Tree* savedCurrentFunction = Tree::getCurrentFunction();

    // ------- Create a temporary empty scope for execution (without copying the body) -------
    SemNode* tmpScopeNode = new SemNode();
    tmpScopeNode->id = ""; tmpScopeNode->DataType = DATA_TYPE::TYPE_SCOPE;
    tmpScopeNode->Param = 0; tmpScopeNode->line = lc.first; tmpScopeNode->col = lc.second;
    // Up of the temporary scope should point to the original function
    Tree* tmpScope = new Tree(tmpScopeNode, fnode);

    // Copy PARAMETERS (if any) — only their declarations and assign them values from args
    Tree* origP = funcScope->Left; // parameters in the original scope
    for (size_t i = 0; origP && i < args.size(); origP = origP->Right, ++i) {
        if (!origP->n) continue;
        SemNode* pcopy = new SemNode(*origP->n); // copy id and type (but not value)
        pcopy->hasValue = false;
        pcopy->line = origP->n->line;
        pcopy->col = origP->n->col;
        // insert parameter into tmpScope as left child of the list (setLeft will add it as Right-sibling)
        tmpScope->setLeft(pcopy);
    }

    // Set parameter values according to args (in order)
    Tree* copyP = tmpScope->Left;

    // inzert zero into func param stack: we would count the arg amount
    funcParamAmountStack.push(0);

    for (size_t i = 0; copyP && i < args.size(); copyP = copyP->Right, ++i)
    {
        if (!copyP->n) continue;

        // Cast argument to the formal parameter type
        SemNode converted = Tree::castToType(args[i], copyP->n->DataType, lc.first, lc.second);

        // Save value and mark as initialized
        copyP->n->hasValue = converted.hasValue;
        copyP->n->isInitialized = true;
        copyP->n->Value = converted.Value;

        // Print implicit conversion warning when debug (like in setVarValue)
        if (args[i].DataType != copyP->n->DataType && Tree::isDebugEnabled())
        {
            Tree::printTypeConversionWarning(
                args[i].DataType,
                copyP->n->DataType,
                "parameter passing",
                copyP->n->id + " in " + fname + "()",
                lc.first,
                lc.second
            );
        }

        // save fresh semantic nodes to currentVariables
        currentVariables[copyP->n->id].push(copyP->n);
        funcAddedVariablesStack.push(copyP->n->id);
        funcParamAmountStack.top()++;

        QString s;
        int l, c;
        std::tie(l, c) = sc->getLineCol();

        auto formatParam = [&](const auto& value, const QString& typeStr)
        {
            return QString("[%1:%2] Evaluated function parameter '%3 %4 = %5'")
            .arg(l)
                .arg(c)
                .arg(typeStr)
                .arg(QString::fromStdString(copyP->n->id))
                .arg(value);
        };

        switch (args[i].DataType)
        {
        case DATA_TYPE::TYPE_SHORT_INT: s = formatParam(copyP->n->Value.v_int16, "short"); break;
        case DATA_TYPE::TYPE_INT:       s = formatParam(copyP->n->Value.v_int32, "int"); break;
        case DATA_TYPE::TYPE_LONG_INT:  s = formatParam(copyP->n->Value.v_int64, "long"); break;
        case DATA_TYPE::TYPE_BOOL:      s = formatParam(copyP->n->Value.v_bool, "bool"); break;
        case DATA_TYPE::TYPE_DOUBLE:    s = formatParam(copyP->n->Value.v_double, "double"); break;
        default: break;
        }

        waitForButtonPress(s);
    }

    // ------- Switch to temporary scope and execute the body --------
    Tree::setCur(tmpScope);
    Tree::setCurrentFunction(fnode);

    // Set scanner to the beginning of the function body (bodyStartPos was saved during parsing)
    size_t bodyStart = (fnode->n) ? fnode->n->bodyStartPos : 0;
    sc->setPos(bodyStart);

    // Execute the function body block (it will be re-parsed, but in the context of tmpScope)
    Block();

    // On exiting the function body — decrement recursion counter
    Tree::exitFunctionCall();

    // Restore context
    sc->setPos(savedPos);
    pushTok = savedPushTok;
    pushLex = savedPushLex;
    curTok = savedCurTok;
    curLex = savedCurLex;
    Tree::setCur(savedCurTree);
    Tree::setCurrentFunction(savedCurrentFunction);

    // remove func params from current variables
    for (int i = 0; i < funcParamAmountStack.top(); i++)
    {
        string const& varName = funcAddedVariablesStack.top();
        currentVariables[varName].pop();
    }
    funcParamAmountStack.pop();

    // Delete temporary scope (it will recursively delete parameters and everything created during execution)
    delete tmpScope;
}
// ArgListOpt -> [Expr (',' Expr)*]
void Diagram::ArgListOpt(vector<SemNode>& args) {
    LexemType t = peekToken();
    if (t != LexemType::RPAREN) {
        DATA_TYPE atype = Expr();
        SemNode arg = popValue();
        args.push_back(arg);

        t = peekToken();
        while (t == LexemType::COMMA) {
            nextToken();
            atype = Expr();
            arg = popValue();
            args.push_back(arg);
            t = peekToken();
        }
    }
}

// Expr -> ['+'|'-'] LogicOr
DATA_TYPE Diagram::Expr()
{
    LexemType t = peekToken();

    bool hasUnary = false;

    string unaryOp = "";

    if (t == LexemType::PLUS || t == LexemType::MINUS)
    {
        LexemType savedTok = nextToken();

        string savedLex = curLex;

        LexemType nextTok = peekToken();

        if (nextTok == LexemType::DEC_LITERAL ||
            nextTok == LexemType::HEX_LITERAL ||
            nextTok == LexemType::DOUBLE_LITERAL)
        {
            pushBack(savedTok, savedLex);
        }
        else
        {
            hasUnary = true;

            unaryOp =
                (savedTok == LexemType::PLUS)
                    ? "+"
                    : "-";
        }
    }

    DATA_TYPE left = LogicOr();

    if (!hasUnary) return left;

    if (!(
            left == DATA_TYPE::TYPE_INT ||
            left == DATA_TYPE::TYPE_SHORT_INT ||
            left == DATA_TYPE::TYPE_LONG_INT ||
            left == DATA_TYPE::TYPE_DOUBLE
            ))
        semError("unary '+'/'-' is only applicable to numeric types");

    SemNode operand = popValue();

    SemNode result;

    if (unaryOp == "-")
    {
        SemNode minusOne;
        minusOne.DataType = left;
        minusOne.hasValue = true;

        switch (left)
        {
        case DATA_TYPE::TYPE_SHORT_INT: minusOne.Value.v_int16 = -1; break;
        case DATA_TYPE::TYPE_INT: minusOne.Value.v_int32 = -1; break;
        case DATA_TYPE::TYPE_LONG_INT: minusOne.Value.v_int64 = -1; break;
        case DATA_TYPE::TYPE_DOUBLE: minusOne.Value.v_double = -1.0; break;
        default: break;
        }

        auto lc = sc->getLineCol();

        result = Tree::executeArithmeticOp(
            operand,
            minusOne,
            "*",
            lc.first,
            lc.second
            );
    }
    else result = operand;
    pushValue(result);

    left = result.DataType;
    return left;
}

// LogicOr -> LogicAnd ( '||' LogicAnd )*
DATA_TYPE Diagram::LogicOr()
{
    DATA_TYPE left = LogicAnd();

    LexemType t = peekToken();

    while (t == LexemType::LOR)
    {
        nextToken();

        DATA_TYPE right = LogicAnd();

        if (left != DATA_TYPE::TYPE_BOOL ||
            right != DATA_TYPE::TYPE_BOOL)
        {
            semError(
                "operands for '||' must be bool"
                );
        }

        SemNode rightVal = popValue();

        SemNode leftVal = popValue();

        SemNode result;

        result.DataType = DATA_TYPE::TYPE_BOOL;
        result.hasValue = true;

        result.Value.v_bool =
            leftVal.Value.v_bool ||
            rightVal.Value.v_bool;

        pushValue(result);

        left = DATA_TYPE::TYPE_BOOL;

        t = peekToken();
    }

    return left;
}

// LogicAnd -> Equality ( '&&' Equality )*
DATA_TYPE Diagram::LogicAnd()
{
    DATA_TYPE left = Equality();

    LexemType t = peekToken();

    while (t == LexemType::LAND)
    {
        nextToken();

        DATA_TYPE right = Equality();

        if (left != DATA_TYPE::TYPE_BOOL ||
            right != DATA_TYPE::TYPE_BOOL)
        {
            semError(
                "operands for '&&' must be bool"
                );
        }

        SemNode rightVal = popValue();

        SemNode leftVal = popValue();

        SemNode result;

        result.DataType = DATA_TYPE::TYPE_BOOL;
        result.hasValue = true;

        result.Value.v_bool =
            leftVal.Value.v_bool &&
            rightVal.Value.v_bool;

        pushValue(result);

        left = DATA_TYPE::TYPE_BOOL;

        t = peekToken();
    }

    return left;
}

// Equality -> Rel ( ('==' | '!=') Rel )*
DATA_TYPE Diagram::Equality()
{
    DATA_TYPE left = Rel();

    LexemType t = peekToken();

    while (t == LexemType::EQ || t == LexemType::NEQ)
    {
        string op =
            (t == LexemType::EQ)
                ? "=="
                : "!=";

        nextToken();

        DATA_TYPE right = Rel();

        SemNode rightVal = popValue();

        SemNode leftVal = popValue();

        bool leftIsInt =
            (left == DATA_TYPE::TYPE_INT ||
             left == DATA_TYPE::TYPE_SHORT_INT ||
             left == DATA_TYPE::TYPE_LONG_INT);

        bool rightIsInt =
            (right == DATA_TYPE::TYPE_INT ||
             right == DATA_TYPE::TYPE_SHORT_INT ||
             right == DATA_TYPE::TYPE_LONG_INT);

        if ((leftIsInt && rightIsInt) ||
            (left == DATA_TYPE::TYPE_BOOL &&
             right == DATA_TYPE::TYPE_BOOL))
        {
            auto lc = sc->getLineCol();

            SemNode result =
                Tree::executeComparisonOp(
                    leftVal,
                    rightVal,
                    op,
                    lc.first,
                    lc.second
                    );

            pushValue(result);

            left = DATA_TYPE::TYPE_BOOL;
        }
        else
        {
            semError(
                "operands for '=='/'!=' must be of the same type"
                );
        }

        t = peekToken();
    }

    return left;
}

// Rel -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
DATA_TYPE Diagram::Rel() {
    DATA_TYPE left = Shift();
    LexemType t = peekToken();

    while (t == LexemType::LT || t == LexemType::LE || t == LexemType::GT || t == LexemType::GE) {
        string op;
        switch (t) {
        case LexemType::LT: op = "<"; break;
        case LexemType::LE: op = "<="; break;
        case LexemType::GT: op = ">"; break;
        case LexemType::GE: op = ">="; break;
        default: break;
        }
        nextToken();
        DATA_TYPE right = Shift();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == DATA_TYPE::TYPE_INT || left == DATA_TYPE::TYPE_SHORT_INT || left == DATA_TYPE::TYPE_LONG_INT);
        bool rInt = (right == DATA_TYPE::TYPE_INT || right == DATA_TYPE::TYPE_SHORT_INT || right == DATA_TYPE::TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands for '<, <=, >, >=' must be integers (int/short/long)");
        }

        auto lc = sc->getLineCol();
        SemNode result = Tree::executeComparisonOp(leftVal, rightVal, op, lc.first, lc.second);
        pushValue(result);
        left = DATA_TYPE::TYPE_BOOL;
        t = peekToken();
    }
    return left;
}

// Shift -> Add ( ('<<' | '>>') Add )*
DATA_TYPE Diagram::Shift() {
    DATA_TYPE left = Add();
    LexemType t = peekToken();

    while (t == LexemType::SHL || t == LexemType::SHR) {
        string op = (t == LexemType::SHL) ? "<<" : ">>";
        nextToken();
        DATA_TYPE right = Add();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == DATA_TYPE::TYPE_INT || left == DATA_TYPE::TYPE_SHORT_INT || left == DATA_TYPE::TYPE_LONG_INT);
        bool rInt = (right == DATA_TYPE::TYPE_INT || right == DATA_TYPE::TYPE_SHORT_INT || right == DATA_TYPE::TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands for shifts must be integers (int/short/long)");
        }

        auto lc = sc->getLineCol();
        SemNode result = Tree::executeShiftOp(leftVal, rightVal, op, lc.first, lc.second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Add -> Mul ( ('+' | '-') Mul )*
DATA_TYPE Diagram::Add() {
    DATA_TYPE left = Mul();
    LexemType t = peekToken();

    while (t == LexemType::PLUS || t == LexemType::MINUS) {
        string op = (t == LexemType::PLUS) ? "+" : "-";
        nextToken();
        DATA_TYPE right = Mul();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == DATA_TYPE::TYPE_INT || left == DATA_TYPE::TYPE_SHORT_INT || left == DATA_TYPE::TYPE_LONG_INT);
        bool rInt = (right == DATA_TYPE::TYPE_INT || right == DATA_TYPE::TYPE_SHORT_INT || right == DATA_TYPE::TYPE_LONG_INT);

        bool lPoint = (left == DATA_TYPE::TYPE_DOUBLE);
        bool rPoint = (right == DATA_TYPE::TYPE_DOUBLE);

        if (!((lInt && rInt) || (lPoint && rPoint))) {
            semError("operands for '+'/'-' must both be integers (int/short/long) or both be floating-point (double)");
        }

        auto lc = sc->getLineCol();
        SemNode result = Tree::executeArithmeticOp(leftVal, rightVal, op, lc.first, lc.second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
DATA_TYPE Diagram::Mul() {
    DATA_TYPE left = Prim();
    LexemType t = peekToken();

    while (t == LexemType::MULT || t == LexemType::DIV || t == LexemType::MOD) {
        string op;
        switch (t) {
        case LexemType::MULT: op = "*"; break;
        case LexemType::DIV: op = "/"; break;
        case LexemType::MOD: op = "%"; break;
        default: break;
        }
        nextToken();
        DATA_TYPE right = Prim();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == DATA_TYPE::TYPE_INT || left == DATA_TYPE::TYPE_SHORT_INT || left == DATA_TYPE::TYPE_LONG_INT);
        bool rInt = (right == DATA_TYPE::TYPE_INT || right == DATA_TYPE::TYPE_SHORT_INT || right == DATA_TYPE::TYPE_LONG_INT);

        bool lPoint = (left == DATA_TYPE::TYPE_DOUBLE);
        bool rPoint = (right == DATA_TYPE::TYPE_DOUBLE);

        if (!((lInt && rInt) || (lPoint && rPoint)))
        {
            semError("operands for '*', '/', '%' must be integers (int/short/long) or both floating-point (double)");
        }

        auto lc = sc->getLineCol();
        SemNode result = Tree::executeArithmeticOp(leftVal, rightVal, op, lc.first, lc.second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Prim -> IDENT | DEC_LITERAL | HEX_LITERAL | true | false | DOUBLE_LITERAL | '(' Expr ')' | Sin | Cos | Deg2Rad | Rad2Deg
DATA_TYPE Diagram::Prim()
{
    LexemType t = nextToken();

    // сначала встроенные функции
    switch (t) {
    case LexemType::KW_SIN: return Sin();
    case LexemType::KW_COS: return Cos();
    case LexemType::KW_TAN: return Tan();
    case LexemType::KW_CTG: return Ctg();
    case LexemType::KW_ARCSIN: return Arcsin();
    case LexemType::KW_ARCCOS: return Arccos();
    case LexemType::KW_ARCTAN: return Arctan();
    case LexemType::KW_ARCCTG: return Arcctg();
    case LexemType::KW_ATAN2: return Atan2();
    case LexemType::KW_PI: return Pi();
    case LexemType::KW_DEG2RAD: return Deg2Rad();
    case LexemType::KW_RAD2DEG: return Rad2Deg();
    case LexemType::KW_COMPARE_DOUBLE: return CompareDouble();
    default: break;
    }

    // First check unary minus for negative literals
    if (t == LexemType::MINUS)
    {
        t = nextToken();
        if (t == LexemType::DEC_LITERAL || t == LexemType::HEX_LITERAL)
        {
            DATA_TYPE literalType = DATA_TYPE::TYPE_SHORT_INT;

            // Determine if we need to automatically promote the type to long
            string valueStr = "-" + curLex;
            try
            {
                long long val = stoll(valueStr, nullptr, 0);

                // If value exceeds short range, automatically make it int
                if (val > 32767 || val < -32768) literalType = DATA_TYPE::TYPE_INT;

                // If value exceeds int range, automatically make it long
                if (val > 2147483647LL || val < -2147483648LL) literalType = DATA_TYPE::TYPE_LONG_INT;
            }
            catch (...) {
                // Leave as DATA_TYPE::TYPE_INT in case of error
            }

            SemNode literalNode = evaluateLiteral("-" + curLex, literalType);
            pushValue(literalNode);
            return literalType;
        }
        else if (t == LexemType::DOUBLE_LITERAL)
        {
            DATA_TYPE literalType = DATA_TYPE::TYPE_DOUBLE;
            string valueStr = "-" + curLex;

            double val = stod(valueStr, nullptr);

            SemNode literalNode = evaluateLiteral("-" + curLex, literalType);
            pushValue(literalNode);
            return literalType;
        }
        else {
            // If minus is not followed by a literal, it's a unary operation on an expression
            // Push the token back and handle as a regular parenthesized expression
            pushBack(t, curLex);
            pushBack(LexemType::MINUS, "-");
            // Handle as parenthesized expression
            t = nextToken();
            if (t != LexemType::LPAREN) {
                synError("expected a literal or parenthesized expression after '-'");
            }
            DATA_TYPE dt = Expr();
            t = nextToken();
            if (t != LexemType::RPAREN) synError("expected ')' after expression");
            return dt;
        }
    }

    if (t == LexemType::DEC_LITERAL || t == LexemType::HEX_LITERAL)
    {
        DATA_TYPE literalType = DATA_TYPE::TYPE_SHORT_INT;

        // Determine if we need to automatically promote the type to long
        string valueStr = curLex;
        try
        {
            long long val = stoll(valueStr, nullptr, 0);

            // If value exceeds short range, automatically make it int
            if (val > 32767 || val < -32768)
            {
                literalType = DATA_TYPE::TYPE_INT;
            }

            // If value exceeds int range, automatically make it long
            if (val > 2147483647LL || val < -2147483648LL)
            {
                literalType = DATA_TYPE::TYPE_LONG_INT;
            }
        }
        catch (...)
        {
            // Leave as DATA_TYPE::TYPE_INT in case of error
        }

        SemNode literalNode = evaluateLiteral(curLex, literalType);
        pushValue(literalNode);
        return literalType;
    }

    if (t == LexemType::DOUBLE_LITERAL)
    {
        DATA_TYPE literalType = DATA_TYPE::TYPE_DOUBLE;
        SemNode literalNode = evaluateLiteral(curLex, literalType);
        pushValue(literalNode);
        return literalType;
    }

    if (t == LexemType::KW_TRUE || t == LexemType::KW_FALSE)
    {
        DATA_TYPE boolType = DATA_TYPE::TYPE_BOOL;
        SemNode boolNode;
        boolNode.DataType = DATA_TYPE::TYPE_BOOL;
        boolNode.hasValue = true;
        boolNode.Value.v_bool = (t == LexemType::KW_TRUE);
        pushValue(boolNode);
        return boolType;
    }

    if (t == LexemType::LPAREN) {
        DATA_TYPE dt = Expr();
        t = nextToken();
        if (t != LexemType::RPAREN) synError("expected ')' after expression");
        return dt;
    }

    if (t == LexemType::IDENT) {
        string name = curLex;
        LexemType t2 = peekToken();

        if (t2 == LexemType::LPAREN) {
            semError("function call inside expression is impossible: functions return void");
        }
        else {
            auto lc = sc->getLineCol();
            Tree* v = Tree::Cur->semGetVar(name, lc.first, lc.second);

            if (!v->n->hasValue) {
                interpError("use of uninitialized variable '" + name + "'");
            }

            SemNode value;
            value.DataType = v->n->DataType;
            value.hasValue = true;
            value.Value = v->n->Value;
            pushValue(value);

            return v->n->DataType;
        }
    }

    synError("expected primary expression (IDENT, literal, or parentheses)");
    return DATA_TYPE::TYPE_INT;
}

// Pi -> 'pi' '(' ')'
DATA_TYPE Diagram::Pi()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function pi");

    SemNode node;
    node.DataType = DATA_TYPE::TYPE_DOUBLE;
    node.hasValue = true;
    node.Value.v_double = PI;
    pushValue(node);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Sin -> 'sin' '(' Expr ')'
DATA_TYPE Diagram::Sin()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function sin");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function sin");

    // pop the angle value in radians from the stack, apply sin, push back onto the stack
    SemNode v = popValue();
    v.Value.v_double = sin(v.Value.v_double);
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Cos -> 'cos' '(' Expr ')'
DATA_TYPE Diagram::Cos()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function cos");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function cos");

    // pop the angle value in radians from the stack, apply cos, push back onto the stack
    SemNode v = popValue();
    v.Value.v_double = cos(v.Value.v_double);
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Tan -> 'tan' '(' Expr ')'
DATA_TYPE Diagram::Tan()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function tan");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function tan");

    // pop the angle value in radians from the stack, apply tan, push back onto the stack
    SemNode v = popValue();

    double const s = std::sin(v.Value.v_double);
    double const c = std::cos(v.Value.v_double);

    if (std::abs(c) < MIN_VAL)
        interpError(tr("Cannot evaluate tan(%1): cos(%1) is close to zero").arg(v.Value.v_double).toStdString());

    double const tan = s/c;
    v.Value.v_double = tan;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Ctg -> 'ctg' '(' Expr ')'
DATA_TYPE Diagram::Ctg()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function ctg");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function ctg");

    // pop the angle value in radians from the stack, apply ctg, push back onto the stack
    SemNode v = popValue();

    double const s = std::sin(v.Value.v_double);
    double const c = std::cos(v.Value.v_double);

    if (std::abs(s) < MIN_VAL)
        interpError(tr("Cannot evaluate ctg(%1): sin(%1) is close to zero").arg(v.Value.v_double).toStdString());

    double const ctg = c/s;
    v.Value.v_double = ctg;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Arcsin -> 'arcsin' '(' Expr ')'
DATA_TYPE Diagram::Arcsin()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function arcsin");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE)
        semError("Only double can be passed to the built-in function arcsin");

    SemNode v = popValue();

    if (std::abs(v.Value.v_double) > 1.0)
        interpError(tr("Cannot evaluate arcsin(%1): value outside domain [-1, 1]").arg(v.Value.v_double).toStdString());

    double const result = std::asin(v.Value.v_double);
    v.Value.v_double = result;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Arccos -> 'arccos' '(' Expr ')'
DATA_TYPE Diagram::Arccos()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function arccos");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE)
        semError("Only double can be passed to the built-in function arccos");

    SemNode v = popValue();

    if (std::abs(v.Value.v_double) > 1.0)
        interpError(tr("Cannot evaluate arccos(%1): value outside domain [-1, 1]").arg(v.Value.v_double).toStdString());

    double const result = std::acos(v.Value.v_double);
    v.Value.v_double = result;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// арктангенс
// Arctan -> 'arctan' '(' Expr ')'
DATA_TYPE Diagram::Arctan()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function arctan");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE)
        semError("Only double can be passed to the built-in function arctan");

    SemNode v = popValue();

    double const result = std::atan(v.Value.v_double);
    v.Value.v_double = result;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// арккотангенс
// Arcctg -> 'arcctg' '(' Expr ')'
DATA_TYPE Diagram::Arcctg()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function arcctg");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE)
        semError("Only double can be passed to the built-in function arcctg");

    SemNode v = popValue();

    double const result = PI / 2.0 - std::atan(v.Value.v_double);
    v.Value.v_double = result;
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Atan2 -> 'atan2' '(' Expr ',' Expr ')'
DATA_TYPE Diagram::Atan2()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function atan2");

    DATA_TYPE dt1 = Expr();
    if (dt1 != DATA_TYPE::TYPE_DOUBLE)
        semError("First argument of atan2 must be of type double");

    t = nextToken();
    if (t != LexemType::COMMA)
        synError("Expected ',' after first argument in atan2");

    DATA_TYPE dt2 = Expr();
    if (dt2 != DATA_TYPE::TYPE_DOUBLE)
        semError("Second argument of atan2 must be of type double");

    SemNode xNode = popValue();
    SemNode yNode = popValue();

    double x = xNode.Value.v_double;
    double y = yNode.Value.v_double;

    // Check for both arguments being zero
    if (std::abs(x) < MIN_VAL && std::abs(y) < MIN_VAL)
        interpError(tr("atan2(%1, %2) is undefined (both arguments are zero)")
            .arg(y).arg(x).toStdString()
        );

    double const result = std::atan2(y, x);

    SemNode resultNode;
    resultNode.DataType = DATA_TYPE::TYPE_DOUBLE;
    resultNode.hasValue = true;
    resultNode.Value.v_double = result;
    pushValue(resultNode);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Deg2Rad -> 'deg2rad' '(' Expr ')'
DATA_TYPE Diagram::Deg2Rad()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function deg2rad");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function deg2rad");

    // pop the angle value in degrees from the stack, apply deg2rad, push the angle value in radians back onto the stack
    SemNode v = popValue();
    v.Value.v_double = deg2rad(v.Value.v_double);
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// Rad2Deg -> 'rad2deg' '(' Expr ')'
DATA_TYPE Diagram::Rad2Deg()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN) synError("Expected '(' after built-in function rad2deg");

    DATA_TYPE dt = Expr();
    if (dt != DATA_TYPE::TYPE_DOUBLE) semError("Only double can be passed to the built-in function rad2deg");

    // pop the angle value in radians from the stack, apply rad2deg, push the angle value in degrees back onto the stack
    SemNode v = popValue();
    v.Value.v_double = rad2deg(v.Value.v_double);
    pushValue(v);

    t = nextToken();
    if (t != LexemType::RPAREN) synError("expected ')' after expression");

    return DATA_TYPE::TYPE_DOUBLE;
}

// compare_double - compares two double values with specified decimal precision
// compare_double(lhs, rhs, decimal) -> returns true if lhs and rhs are equal within the specified decimal places
DATA_TYPE Diagram::CompareDouble()
{
    LexemType t = nextToken();
    if (t != LexemType::LPAREN)
        synError("Expected '(' after built-in function compare_double");

    DATA_TYPE dt1 = Expr();
    if (dt1 != DATA_TYPE::TYPE_DOUBLE)
        semError("First argument of compare_double must be of type double");

    t = nextToken();
    if (t != LexemType::COMMA)
        synError("Expected ',' after first argument in compare_double");

    DATA_TYPE dt2 = Expr();
    if (dt2 != DATA_TYPE::TYPE_DOUBLE)
        semError("Second argument of compare_double must be of type double");

    t = nextToken();
    if (t != LexemType::COMMA)
        synError("Expected ',' after second argument in compare_double");

    DATA_TYPE dt3 = Expr();
    if (!(
            dt3 == DATA_TYPE::TYPE_SHORT_INT ||
            dt3 == DATA_TYPE::TYPE_INT ||
            dt3 == DATA_TYPE::TYPE_LONG_INT)
        )
        semError("Third argument of compare_double must be an integer (number of decimal places)");

    SemNode decimalNode = popValue();
    SemNode rhsNode = popValue();
    SemNode lhsNode = popValue();

    double const lhs = lhsNode.Value.v_double;
    double const rhs = rhsNode.Value.v_double;

    int64_t decimal;
    switch (dt3)
    {
    case DATA_TYPE::TYPE_SHORT_INT: decimal = decimalNode.Value.v_int16; break;
    case DATA_TYPE::TYPE_INT: decimal = decimalNode.Value.v_int32; break;
    case DATA_TYPE::TYPE_LONG_INT: decimal = decimalNode.Value.v_int64; break;
    default: interpError("Internal error occured when executing compare_double built-in function");
    }

    if (decimal < 0)
        interpError("Decimal places argument in compare_double cannot be negative");

    double epsilon = std::pow(10.0, -decimal);

    double diff = lhs - rhs;

    SemNode resultNode;
    resultNode.DataType = DATA_TYPE::TYPE_SHORT_INT;
    resultNode.hasValue = true;
    resultNode.Value.v_int16 = (std::abs(diff) < epsilon ? 0 : diff > 0 ? 1 : -1);
    pushValue(resultNode);

    t = nextToken();
    if (t != LexemType::RPAREN)
        synError("expected ')' after expression");

    return DATA_TYPE::TYPE_SHORT_INT;
}

// IfStmt -> 'if' '(' Expr ')' Stmt ['else' Stmt]
void Diagram::IfStmt()
{
    bool const interpretFlagLocal = Tree::isInterpretationEnabled();

    LexemType t = nextToken(); // consumed KW_IF
    if (t != LexemType::KW_IF) synError("Expected keyword 'if'");

    t = nextToken(); // '('
    if (t != LexemType::LPAREN) synError("Expected '(' after keyword 'if'");

    DATA_TYPE dtype = Expr();
    if (dtype != DATA_TYPE::TYPE_BOOL) semError("Condition in if statement must be of type bool");
    SemNode n = popValue();

    if (Tree::isInterpretationEnabled() && n.Value.v_bool)
        Tree::enableInterpretation();
    else
        Tree::disableInterpretation();

    t = nextToken(); // ')'
    if (t != LexemType::RPAREN) synError("Expected ')' after expression");

    Stmt();

    if (interpretFlagLocal)
    {
        if (Tree::isInterpretationEnabled())
            Tree::disableInterpretation();
        else
            Tree::enableInterpretation();
    }

    if (peekToken() != LexemType::KW_ELSE) return;
    else nextToken(); // 'else'

    Stmt();

    if (interpretFlagLocal)
        Tree::enableInterpretation();
    else
        Tree::disableInterpretation();
}

// WhileStmt -> 'while' '(' Expr ')' Stmt
void Diagram::WhileStmt()
{
    bool const interpretFlagLocal = Tree::isInterpretationEnabled();

    LexemType t = nextToken(); // consumed KW_WHILE
    if (t != LexemType::KW_WHILE) synError("Expected keyword 'while'");

    size_t const pos = sc->getPos();

StartWhile:
    t = nextToken(); // '('
    if (t != LexemType::LPAREN) synError("Expected '(' after keyword 'while'");

    DATA_TYPE dtype = Expr();
    if (dtype != DATA_TYPE::TYPE_BOOL) semError("Condition expression in 'while' loop must be of type bool");
    SemNode n = popValue();

    if (Tree::isInterpretationEnabled() && n.Value.v_bool)
        Tree::enableInterpretation();
    else
        Tree::disableInterpretation();

    t = nextToken(); // ')'
    if (t != LexemType::RPAREN) synError("Expected ')' after expression");

    Stmt();

    if (Tree::isInterpretationEnabled())
    {
        sc->setPos(pos);
        goto StartWhile;
    }

    if (interpretFlagLocal)
        Tree::enableInterpretation();
    else
        Tree::disableInterpretation();
}

// MoveStmt -> 'Move' '(' Expr ',' Expr ')' ';'
void Diagram::MoveStmt()
{
    LexemType t = nextToken(); // KW_MOVE
    if (t != LexemType::KW_MOVE) synError("Expected keyword 'Move'");

    t = nextToken(); // '('
    if (t != LexemType::LPAREN) synError("Expected '(' after keyword 'Move'");

    DATA_TYPE dtype1 = Expr();
    if (dtype1 != DATA_TYPE::TYPE_DOUBLE) semError("Move command argument must be of type double");

    t = nextToken(); // ','
    if (t != LexemType::COMMA) synError("Expected ',' after expression");

    DATA_TYPE dtype2 = Expr();
    if (dtype2 != DATA_TYPE::TYPE_DOUBLE) semError("Move command argument must be of type double");

    t = nextToken();
    if (t != LexemType::RPAREN) synError("Expected ')' after second argument in 'Move' command");

    SemNode y_offset = popValue();
    SemNode x_offset = popValue();

    if (Tree::isInterpretationEnabled())
        executeMove(x_offset, y_offset);

    t = nextToken(); // ';'
    if (t != LexemType::SEMI) synError("Expected ';' after Move command");
}

// rotation:
// joint index
// normalX
// normalY
// normalZ
// angle (radians)
// RotateStmt -> 'Rotate' '(' Expr ',' Expr ',' Expr ',' Expr ',' Expr ')' ';'
void Diagram::RotateStmt()
{
    LexemType t = nextToken(); // KW_ROTATE
    if (t != LexemType::KW_ROTATE) synError("Expected keyword 'Rotate'");

    t = nextToken(); // '('
    if (t != LexemType::LPAREN) synError("Expected '(' after keyword 'Rotate'");

    DATA_TYPE dtype1 = Expr();
    if (!(dtype1 == DATA_TYPE::TYPE_SHORT_INT)) semError("First argument of Rotate command must be of type short");
    SemNode robotNodeIdx = popValue();

    t = nextToken(); // ','
    if (t != LexemType::COMMA) synError("Expected ',' after expression");

    DATA_TYPE dtype2 = Expr();
    if (dtype2 != DATA_TYPE::TYPE_DOUBLE) semError("Second argument of Rotate command must be of type double");
    SemNode normalX = popValue();

    t = nextToken(); // ','
    if (t != LexemType::COMMA) synError("Expected ',' after expression");

    DATA_TYPE dtype3 = Expr();
    if (dtype3 != DATA_TYPE::TYPE_DOUBLE) semError("Third argument of Rotate command must be of type double");
    SemNode normalY = popValue();

    t = nextToken(); // ','
    if (t != LexemType::COMMA) synError("Expected ',' after expression");

    DATA_TYPE dtype4 = Expr();
    if (dtype4 != DATA_TYPE::TYPE_DOUBLE) semError("Fourth argument of Rotate command must be of type double");
    SemNode normalZ = popValue();

    t = nextToken(); // ','
    if (t != LexemType::COMMA) synError("Expected ',' after expression");

    DATA_TYPE dtype5 = Expr();
    if (dtype5 != DATA_TYPE::TYPE_DOUBLE) semError("Fifth argument of Rotate command must be of type double");
    SemNode radians = popValue();

    t = nextToken(); // ')'
    if (t != LexemType::RPAREN) synError("Expected ')' after second argument in 'Rotate' command");

    if (Tree::isInterpretationEnabled())
        executeRotate(robotNodeIdx, normalX, normalY, normalZ, radians);

    t = nextToken(); // ';'
    if (t != LexemType::SEMI) synError("Expected ';' after Rotate command");
}

void Diagram::executeMove(
    SemNode const& x_offset,
    SemNode const& y_offset)
{
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        auto& rd = RobotData::getInstance();
        double const x = rd.getX(i);
        double const y = rd.getY(i);
        rd.setX(i, x + x_offset.Value.v_double);
        rd.setY(i, y + y_offset.Value.v_double);
    }

    int l, c;
    tie(l, c) = sc->getLineCol();

    waitForButtonPress(QString("[%1:%2] Move(%3, %4) executed")
                           .arg(l)
                           .arg(c)
                           .arg(x_offset.Value.v_double)
                           .arg(y_offset.Value.v_double));
    checkJointsInsideWorkspace();
}

void Diagram::executeRotate(
    SemNode const& robotNodeIdx,
    SemNode const& normalX,
    SemNode const& normalY,
    SemNode const& normalZ,
    SemNode const& radians)
{
    if (robotNodeIdx.Value.v_int16 >= RobotData::segmentAmount)
        interpError(QString("Joint index for rotation (%1) must be less than the robot's segment amount (%2)")
                        .arg(robotNodeIdx.Value.v_int16)
                        .arg(RobotData::segmentAmount)
                        .toStdString()
        );

    double const len = sqrt(
        normalX.Value.v_double * normalX.Value.v_double +
        normalY.Value.v_double * normalY.Value.v_double +
        normalZ.Value.v_double * normalZ.Value.v_double
        );

    if (len < MIN_VAL)
    {
        QString err_str(QString("Normal length in Rotate command is too small (%1 < %2)").arg(len).arg(MIN_VAL));
        interpError(err_str.toStdString());
    }

    auto& rd = RobotData::getInstance();

    rd.rotate(
        robotNodeIdx.Value.v_int16,
        normalX.Value.v_double,
        normalY.Value.v_double,
        normalZ.Value.v_double,
        radians.Value.v_double
        );


    int l, c;
    tie(l, c) = sc->getLineCol();

    QString const s = QString("[%1:%2] Rotate(%3, %4, %5, %6, %7) executed")
                          .arg(l)
                          .arg(c)
                          .arg(robotNodeIdx.Value.v_int16)
                          .arg(normalX.Value.v_double)
                          .arg(normalY.Value.v_double)
                          .arg(normalZ.Value.v_double)
                          .arg(radians.Value.v_double);
    checkJointsInsideWorkspace();
    waitForButtonPress(s);
}

void Diagram::startParse()
{
    try
    {
        checkJointsInsideWorkspace();
        waitForButtonPress("Parsing successfully began!");
        ParseProgram(true, true);
        waitForButtonPress("Parsing successfully finished!");
        emit parseFinished();
    }
    catch (const abrupt_parse_finish& ex)
    {
        // не бросаем сигнал; разделение причины исключения
    }
    catch (const std::exception& ex)
    {
        QString err_msg = QString::fromUtf8(ex.what());
        emit parseError(err_msg);
    }
    catch (...)
    {
        emit parseError("Unknown parser error");
    }
    QThread::currentThread()->quit();
}

void Diagram::waitForButtonPress(QString const& currentStepMsg)
{
    QMutexLocker locker(&stepMutex);

    emit sendAllCurrentVariables(getAllCurrentVariables());
    emit sendRobotData();
    emit sendCurrentStepMsg(currentStepMsg);

    while (!buttonPressed && !stopRequested)
        stepCondition.wait(&stepMutex);

    buttonPressed = false;

    if (stopRequested)
        throw abrupt_parse_finish("Parser forcefully stopped");
}

void Diagram::nextStep()
{
    QMutexLocker locker(&stepMutex);
    buttonPressed = true;
    stepCondition.wakeOne();
}

void Diagram::requestStop()
{
    QMutexLocker locker(&stepMutex);
    stopRequested = true;
    buttonPressed = true;
    stepCondition.wakeOne();
}

void Diagram::checkJointsInsideWorkspace()
{
    auto& rd = RobotData::getInstance();
    QString msg;
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        if (rd.isJointInsideWorkspace(i)) continue;

        double const x = rd.getX(i);
        double const y = rd.getY(i);
        double const z = rd.getZ(i);

        msg += QString(
                   "\nJoint #%1 with coordinates [%2 %3 %4] was moved out of workspace")
                   .arg(i+1)
                   .arg(x)
                   .arg(y)
                   .arg(z);
    }
    if (!msg.isEmpty()) interpError((msg + "\n").toStdString());
}
