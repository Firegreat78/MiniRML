// Diagram.h
#ifndef DIAGRAM_H
#define DIAGRAM_H

#include "scanner.h"
#include "SemNode.h"
#include "DataType.h"
#include "RobotData.h"
#include "tree.h"

#include <string>
#include <vector>
#include <stack>

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

class Diagram : public QObject
{
    Q_OBJECT

private:
    Scanner* sc;
    std::vector<LexemType> pushTok;
    std::vector<std::string> pushLex;
    LexemType curTok;
    std::string curLex;
    DATA_TYPE currentDeclType;

    // Стек для вычисления выражений
    std::stack<SemNode> evalStack;

    // текущие переменные
    std::map<std::string, std::stack<SemNode*>> currentVariables;
    std::vector<std::pair<std::string, SemNode*>> getAllCurrentVariables();

    // переменные-параметры функций
    std::stack<int> funcParamAmountStack;
    std::stack<std::string> funcAddedVariablesStack;

    // переменные декларированные в scope
    std::stack<int> scopeInitializedVarsAmountStack;
    std::stack<std::string> scopeAddedVariablesStack;

    // Для пошагового выполнения в отдельном потоке
    QMutex stepMutex;
    QWaitCondition stepCondition;
    bool buttonPressed = false;
    bool stopRequested = false;

    LexemType nextToken();
    LexemType peekToken();
    void pushBack(LexemType tok, const std::string& lex);
    void lexError();
    void synError(const std::string& msg);
    void semError(const std::string& msg);
    void interpError(const std::string& msg);

    // Синтаксические процедуры с интерпретацией
    void Program();
    void TopDecl();
    void Function();
    void VarDecl();
    void IdInitList();
    void IdInit();
    void Block();
    void BlockItems();
    void Stmt();
    void Assign();
    void CallStmt();
    void SwitchStmt();
    bool CaseStmt(long long switchVal, bool& anyMatched);
    bool DefaultStmt(bool anyMatched);
    void IfStmt();
    void WhileStmt();

    // команды для робота
    void MoveStmt(); // движение нижнего шарнира
    void RotateStmt(); // поворот

    // Выражения с интерпретацией
    DATA_TYPE Expr();
    DATA_TYPE LogicOr(); // логическое ИЛИ
    DATA_TYPE LogicAnd(); // логическое И
    DATA_TYPE Equality(); // сравнение (равно/не равно)
    DATA_TYPE Rel(); // сравнение (больше/меньше)
    DATA_TYPE Shift(); // битовые сдвиги
    DATA_TYPE Add(); // сложение/вычитание
    DATA_TYPE Mul(); // умножение/деление/modulo
    DATA_TYPE Prim(); // элементарное выражение
    DATA_TYPE Pi(); // число пи ()
    DATA_TYPE Sin(); // синус
    DATA_TYPE Cos(); // косинус
    DATA_TYPE Tan(); // тангенс
    DATA_TYPE Ctg(); // котангенс
    DATA_TYPE Arcsin(); // арксинус
    DATA_TYPE Arccos(); // арккосинус
    DATA_TYPE Deg2Rad(); // градусы -> радианы
    DATA_TYPE Rad2Deg(); // радианы -> градусы
    void Call(); // вызов функции
    void ArgListOpt(std::vector<SemNode>& args); // параметры функции

    // Вспомогательные методы интерпретации
    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateLiteral(const std::string& value, DATA_TYPE type);
    void executeAssignment(const std::string& varName, DATA_TYPE exprType, int line, int col);
    void executeMove(SemNode const&, SemNode const&);
    void executeRotate(SemNode const&, SemNode const&, SemNode const&, SemNode const&, SemNode const&);
    void checkJointsInsideWorkspace();

public:
    Diagram(Scanner* scanner);
    void ParseProgram(bool isInterp = true, bool isDebug = false);
    void requestStop();

public slots:
    void startParse();
    void nextStep();
private:
    void waitForButtonPress(QString const&);

signals:
    // для передачи всех значений переменных
    void sendAllCurrentVariables(std::vector<std::pair<std::string, SemNode*>>);

    // для передачи координат шарниров робота
    void sendRobotData();

    // для информации о текущем шаге
    void sendCurrentStepMsg(QString const&);

    // при любой ошибке, кидаем сюда текст ошибки чтобы было видно в gui
    void parseError(QString msg);

    // конец
    void parseFinished();
};

#endif // DIAGRAM_H
