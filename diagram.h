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

    enum class HitCoordinateType : uint8_t
    {
        X, Y, Z
    };

    Scanner* sc;
    std::vector<LexemType> pushTok;
    std::vector<std::string> pushLex;
    LexemType curTok;
    std::string curLex;
    DATA_TYPE currentDeclType;

    // Стек для вычисления выражений
    std::stack<SemNode> evalStack;

    // текущие переменные
    std::map<std::size_t, std::stack<SemNode*>> currentVariables;
    std::map<std::size_t, std::stack<SemNode*>>const *const getAllCurrentVariables() const;

    // переменные-параметры функций
    std::stack<int> funcParamAmountStack;
    std::stack<decltype(currentVariables)::key_type> funcAddedVariablesStack;

    // переменные декларированные в scope
    std::stack<int> scopeInitializedVarsAmountStack;
    std::stack<decltype(currentVariables)::key_type> scopeAddedVariablesStack;

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

    // программа: итерация описаний в глобальной области
    // Program -> (TopDecl)*
    void Program();

    // описание в глобальной области: функция или переменные
    // TopDecl -> Function | VarDecl
    void TopDecl();

    // описание функции
    // Function -> 'void' IDENT '(' ParamListOpt ')' Block
    void Function();

    // декларация переменных
    // VarDecl -> Type IdInitList ';'
    void VarDecl();

    // список декларарируемых переменных
    // IdInitList -> IdInit (',' IdInit)*
    void IdInitList();

    // декларация переменной определённого типа
    // IdInit -> IDENT [ '=' Expr ]
    void IdInit();

    /* [Область видимости] */
    // составной оператор
    // Block -> '{' BlockItems '}'
    void Block();

    // BlockItems -> (VarDecl | Stmt)*
    // описание переменных или оператор
    void BlockItems();
    /* [Область видимости] */

    /* [Операторы] */
    // оператор
    // Stmt -> ';' | Block | Assign | CallStmt | SwitchStmt | WhileStmt | IfStmt | MoveStmt | RotateStmt
    void Stmt();

    // присваивание
    // Assign -> IDENT '=' Expr
    void Assign();

    // вызов функции
    // CallStmt -> Call ';'
    void CallStmt();

    // switch
    // SwitchStmt -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
    void SwitchStmt();

    // ветка в switch
    // CaseStmt -> 'case' Literal ':' Stmt*
    bool CaseStmt(long long switchVal, bool& anyMatched);

    // DefaultStmt -> 'default' ':' Stmt*
    bool DefaultStmt(bool anyMatched);

    // IfStmt -> 'if' '(' Expr ')' Stmt ['else' Stmt]
    void IfStmt();

    // WhileStmt -> 'while' '(' Expr ')' Stmt
    void WhileStmt();
    /* [Операторы] */

    /* [Команды для управления роботом] */
    // команда для поступательного смещения робота в плоскости XOY
    // MoveStmt -> 'Move' '(' Expr ',' Expr ')' ';'
    void MoveStmt();

    // команда для поворота робота за шарнир
    // RotateStmt -> 'Rotate' '(' Expr ',' Expr ',' Expr ',' Expr ',' Expr ')' ';'
    void RotateStmt();
    /* [Команды для управления роботом] */

    /* [Выражения] */
    // выражение
    // Expr -> ['+'|'-'] LogicOr
    DATA_TYPE Expr();

    // логическое ИЛИ
    // LogicOr -> LogicAnd ( '||' LogicAnd )*
    DATA_TYPE LogicOr();

    // логическое И
    // LogicAnd -> Equality ( '&&' Equality )*
    DATA_TYPE LogicAnd();

    // сравнение
    // Equality -> Rel ( ('==' | '!=') Rel )*
    DATA_TYPE Equality();

    // сравнение
    // Rel -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
    DATA_TYPE Rel();

    // битовые сдвиги
    // Shift -> Add ( ('<<' | '>>') Add )*
    DATA_TYPE Shift();

    // сложение/вычитание
    // Add -> Mul ( ('+' | '-') Mul )*
    DATA_TYPE Add();

    // умножение/деление/modulo
    // Mul -> Prim ( ('*' | '/' | '%') Prim )*
    DATA_TYPE Mul();

    // элементарное выражение
    // Prim -> IDENT
    //       | DEC_LITERAL
    //       | HEX_LITERAL
    //       | DOUBLE_LITERAL
    //       | KW_TRUE
    //       | KW_FALSE
    //       | '(' Expr ')'
    //       | Sin
    //       | Cos
    //       | Tan
    //       | Ctg
    //       | Arcsin
    //       | Arccos
    //       | Arctan
    //       | Arcctg
    //       | Atan2
    //       | Pi
    //       | SegmentAmount
    //       | SegmentLength
    //       | WorkspaceSize
    //       | GetX
    //       | GetY
    //       | GetZ
    //       | HitX
    //       | HitY
    //       | HitZ
    //       | Deg2Rad
    //       | Rad2Deg
    DATA_TYPE Prim();
    /* [Выражения] */

    /* [Встроенные функции] */
    // число пи
    // Pi -> 'pi' '(' ')'
    DATA_TYPE Pi();

    // количество шарниров робота
    // SegmentAmount -> 'segment_amount' '(' ')'
    DATA_TYPE SegmentAmount();

    // длина указанного сегмента
    // SegmentLength -> 'segment_length' '(' Expr ')'
    DATA_TYPE SegmentLength();

    // размер рабочей области
    // WorkspaceSize -> 'workspace_size' '(' ')'
    DATA_TYPE WorkspaceSize();

    // Координата X указанного шарнира
    // GetX -> 'get_x' '(' Expr ')'
    DATA_TYPE GetX();

    // Координата Y указанного шарнира
    // GetY -> 'get_y' '(' Expr ')'
    DATA_TYPE GetY();

    // Координата Z указанного шарнира
    // GetZ -> 'get_z' '(' Expr ')'
    DATA_TYPE GetZ();

    // получить координату X|Y|Z первого удара о стенку рабочей области при вращении с заданными характеристиками
    // Hit -> HitX | HitY | HitZ
    DATA_TYPE Hit(HitCoordinateType);

    // получить координату X первого удара о стенку рабочей области при вращении с заданными характеристиками
    // HitX -> 'hitx' '(' Expr ',' Expr ',' Expr ',' Expr ',' Expr ',' Expr ')'
    DATA_TYPE HitX();

    // получить координату X первого удара о стенку рабочей области при вращении с заданными характеристиками
    // HitY -> 'hity' '(' Expr ',' Expr ',' Expr ',' Expr ',' Expr ',' Expr ')'
    DATA_TYPE HitY();

    // получить координату X первого удара о стенку рабочей области при вращении с заданными характеристиками
    // HitZ -> 'hitz' '(' Expr ',' Expr ',' Expr ',' Expr ',' Expr ',' Expr ')'
    DATA_TYPE HitZ();

    // синус
    // Sin -> 'sin' '(' Expr ')'
    DATA_TYPE Sin();

    // косинус
    // Cos -> 'cos' '(' Expr ')'
    DATA_TYPE Cos();

    // тангенс
    // Tan -> 'tan' '(' Expr ')'
    DATA_TYPE Tan();

    // котангенс
    // Ctg -> 'ctg' '(' Expr ')'
    DATA_TYPE Ctg();

    // арксинус
    // Arcsin -> 'arcsin' '(' Expr ')'
    DATA_TYPE Arcsin();

    // арккосинус
    // Arccos -> 'arccos' '(' Expr ')'
    DATA_TYPE Arccos();

    // арктангенс
    // Arctan -> 'arctan' '(' Expr ')'
    DATA_TYPE Arctan();

    // арккотангенс
    // Arctan -> 'arcctg' '(' Expr ')'
    DATA_TYPE Arcctg();

    // арктангенс двух аргументов (y, x)
    // Atan2 -> 'atan2' '(' Expr ',' Expr ')'
    DATA_TYPE Atan2();

    // градусы -> радианы
    // Deg2Rad -> 'deg2rad' '(' Expr ')'
    DATA_TYPE Deg2Rad();

    // радианы -> градусы
    // Rad2Deg -> 'rad2deg' '(' Expr ')'
    DATA_TYPE Rad2Deg();

    // сравнение двух double с заданной точностью
    // CompareDouble -> 'compare_double' '(' Expr ',' Expr ',' Expr ')'
    // вернёт 0, если числа равны с учётом точности
    // вернёт 1, если первое число больше второго
    // вернёт -1, если первое число меньше первого
    DATA_TYPE CompareDouble();
    /* [Встроенные функции] */

    /* [Вызов пользовательских функций] */
    // вызов функции
    // Call -> IDENT '(' ArgListOpt ')'
    void Call();

    // параметры функции
    // ArgListOpt -> [Expr (',' Expr)*]
    void ArgListOpt(std::vector<SemNode>& args);
    /* [Вызов пользовательских функций] */

    /* [Вспомогательные методы интерпретации] */
    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateLiteral(const std::string& value, DATA_TYPE type);
    void executeAssignment(const std::string& varName, DATA_TYPE exprType, int line, int col);
    void executeMove(SemNode const&, SemNode const&);
    void executeRotate(SemNode const&, SemNode const&, SemNode const&, SemNode const&, SemNode const&);
    void checkJointsInsideWorkspace();
    /* [Вспомогательные методы интерпретации] */
public:
    Diagram(Scanner* scanner);
    void ParseProgram(bool isInterp = true, bool isDebug = false);
    void requestStop();

public slots:
    // запуск парсера (при запуске потока)
    void startParse();

    // следующий шаг
    void nextStep();
private:
    // вызываем для отметки шага, парсинг остановится пока не вызовем nextStep
    // nextStep вызывается при активации кнопки следующего шага в gui
    void waitForButtonPress(QString const&);

signals:
    // для передачи всех текущих значений переменных (с учётом name shadowing)
    void sendAllCurrentVariables(std::map<std::size_t, std::stack<SemNode*>>const *const);

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
