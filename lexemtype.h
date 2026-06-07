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
    KW_SIN,        // синус
    KW_COS,        // косинус
    KW_TAN,        // тангенс
    KW_CTG,        // котангенс
    KW_ARCSIN,     // арксинус
    KW_ARCCOS,     // арккосинус
    KW_ARCTAN,     // арктангенс
    KW_ARCCTG,     // арккотангенс
    KW_ATAN2,      // арктангенс двух аргументов
    KW_PI,         // число пи
    KW_DEG2RAD,    // градусы -> радианы
    KW_RAD2DEG,    // радианы -> градусы
    KW_COMPARE_DOUBLE, // сравнение двух чисел с плавающей запятой

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
