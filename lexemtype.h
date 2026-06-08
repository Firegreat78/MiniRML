#ifndef LEXEMTYPE_H
#define LEXEMTYPE_H

enum class LexemType
{
    EMPTY,

    // Ключевые слова
    KW_INT,
    KW_SHORT,
    KW_LONG,
    KW_DOUBLE,
    KW_BOOL,
    KW_VOID,
    KW_SWITCH,
    KW_CASE,
    KW_DEFAULT,
    KW_BREAK,
    KW_TRUE,
    KW_FALSE,
    KW_MAIN,
    KW_IF,
    KW_ELSE,
    KW_WHILE,

    // встроенные команды
    KW_MOVE,
    KW_ROTATE,

    // встроенные функции
    KW_SIN,              // sin(x) - синус
    KW_COS,              // cos(x) - косинус
    KW_TAN,              // tan(x) - тангенс
    KW_CTG,              // ctg(x) - котангенс
    KW_ARCSIN,           // arcsin(x) - арксинус
    KW_ARCCOS,           // arccos(x) - арккосинус
    KW_ARCTAN,           // arctan(x) - арктангенс
    KW_ARCCTG,           // arcctg(x) - арккотангенс
    KW_ATAN2,            // atan2(y, x) - арктангенс двух аргументов
    KW_PI,               // pi() - число π
    KW_SEGMENT_AMOUNT,   // segment_amount() - количество сегментов робота
    KW_SEGMENT_LENGTH,   // segment_length(idx) - длина сегмента
    KW_GET_X,            // get_x(idx) - координата X шарнира
    KW_GET_Y,            // get_y(idx) - координата Y шарнира
    KW_GET_Z,            // get_z(idx) - координата Z шарнира
    KW_HIT_X,
    KW_HIT_Y,
    KW_HIT_Z,
    KW_WORKSPACE_SIZE,   // workspace_size() - размер рабочей области
    KW_DEG2RAD,          // deg2rad(x) - градусы в радианы
    KW_RAD2DEG,          // rad2deg(x) - радианы в градусы
    KW_COMPARE_DOUBLE,   // compare_double(lhs, rhs, dec) - сравнение double с точностью


    // Идентификатор
    IDENT,

    // Литералы
    DEC_LITERAL, // обычная десятичная (0, 4, 540, ...)
    HEX_LITERAL, // 0x...
    BOOL_LITERAL, // true, false
    DOUBLE_LITERAL, // 1e4, 1e+4, 1e-4, 0.4

    // Разделители
    SEMI, // ;
    COMMA, // ,
    LPAREN, // (
    RPAREN, // )
    LBRACE, // {
    RBRACE, // }
    COLON, // :

    // Операторы
    LAND, // &&
    LOR,  // ||
    EQ, // ==
    NEQ, // !=
    LE, // <=
    GE, // >=
    SHL, // <<
    SHR, // >>
    LT, // <
    GT, // >
    ASSIGN, // =
    PLUS, // +
    MINUS, // -
    MULT, // *
    DIV, // /
    MOD, // %

    // Специальные
    T_END,
    T_ERR
};
#endif // LEXEMTYPE_H
