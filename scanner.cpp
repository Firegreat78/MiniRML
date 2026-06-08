// scanner.cpp
#include "scanner.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

constexpr size_t MAX_NUMERIC_LITERAL_LEN = 20;

using namespace std;

// В конструкторе текущая позиция = 0, текст пуст
Scanner::Scanner() : text(), currentPos(0) {}

bool Scanner::loadFile(const string& fileName) {
    ifstream in(fileName);
    if (!in) return false;

    ostringstream ss;
    ss << in.rdbuf();
    text = ss.str();

    text.push_back('\0');
    currentPos = 0;
    return true;
}

// Работа с текущим символом
char Scanner::peek(size_t offset) const {
    size_t pos = currentPos + offset;
    if (pos < text.size()) return text[pos];
    return '\0';
}
char Scanner::getChar() {
    if (currentPos < text.size()) return text[currentPos++];
    return '\0';
}
void Scanner::ungetChar() {
    if (currentPos > 0) --currentPos;
}

// Всевозможные классификации символов
bool Scanner::isLetter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool Scanner::isDigit(char c) {
    return (c >= '0' && c <= '9');
}
bool Scanner::isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
bool Scanner::isIdentStart(char c) {
    return isLetter(c) || c == '_';
}
bool Scanner::isIdentPart(char c) {
    return isLetter(c) || isDigit(c) || c == '_';
}

// Проверка ключевых слов: возвращает соответствующий KW_* или IDENT
LexemType Scanner::checkKeyword(const string& s) {
    if (s == "int") return LexemType::KW_INT;
    if (s == "short") return LexemType::KW_SHORT;
    if (s == "long") return LexemType::KW_LONG;
    if (s == "double") return LexemType::KW_DOUBLE;
    if (s == "bool") return LexemType::KW_BOOL;
    if (s == "void") return LexemType::KW_VOID;
    if (s == "switch") return LexemType::KW_SWITCH;
    if (s == "case") return LexemType::KW_CASE;
    if (s == "default") return LexemType::KW_DEFAULT;
    if (s == "break") return LexemType::KW_BREAK;
    if (s == "true") return LexemType::KW_TRUE;
    if (s == "false") return LexemType::KW_FALSE;
    if (s == "main") return LexemType::KW_MAIN;
    if (s == "if") return LexemType::KW_IF;
    if (s == "else") return LexemType::KW_ELSE;
    if (s == "while") return LexemType::KW_WHILE;
    if (s == "Move") return LexemType::KW_MOVE;
    if (s == "Rotate") return LexemType::KW_ROTATE;
    if (s == "sin") return LexemType::KW_SIN;
    if (s == "cos") return LexemType::KW_COS;
    if (s == "tan") return LexemType::KW_TAN;
    if (s == "ctg") return LexemType::KW_CTG;
    if (s == "arcsin") return LexemType::KW_ARCSIN;
    if (s == "arccos") return LexemType::KW_ARCCOS;
    if (s == "arctan") return LexemType::KW_ARCTAN;
    if (s == "arcctg") return LexemType::KW_ARCCTG;
    if (s == "atan2") return LexemType::KW_ATAN2;
    if (s == "pi") return LexemType::KW_PI;
    if (s == "segment_amount") return LexemType::KW_SEGMENT_AMOUNT;
    if (s == "segment_length") return LexemType::KW_SEGMENT_LENGTH;
    if (s == "get_x") return LexemType::KW_GET_X;
    if (s == "get_y") return LexemType::KW_GET_Y;
    if (s == "get_z") return LexemType::KW_GET_Z;
    if (s == "hitx") return LexemType::KW_HIT_X;
    if (s == "hity") return LexemType::KW_HIT_Y;
    if (s == "hitz") return LexemType::KW_HIT_Z;
    if (s == "workspace_size") return LexemType::KW_WORKSPACE_SIZE;
    if (s == "deg2rad") return LexemType::KW_DEG2RAD;
    if (s == "rad2deg") return LexemType::KW_RAD2DEG;
    if (s == "compare_double") return LexemType::KW_COMPARE_DOUBLE;
    return LexemType::IDENT;
}

// Пропустить пробелы и комментарии (// и /* ... */)
void Scanner::skipIgnored() {
    for (;;) {
        char c = peek();
        // пробелы / табуляция / перевод строки / возврат каретки
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getChar();
            continue;
        }
        if (c == '/') {
            char n = peek(1);
            if (n == '/') {
                // однострочный комментарий: убираем '//' и читаем до следующей строки
                getChar(); getChar();
                while (peek() != '\n' && peek() != '\0') getChar();
                continue;
            }
            else if (n == '*') {
                // блочный комментарий /* ... */
                getChar(); getChar(); // убираем '/*'
                bool closed = false;
                while (peek() != '\0') {
                    char ch = getChar();
                    if (ch == '*' && peek() == '/') {
                        getChar(); // убираем '/'
                        closed = true;
                        break;
                    }
                }
                if (!closed) {
                    // незакрытый комментарий
                    return;
                }
                continue;
            }
        }
        break; // нет игнорируемых символов
    }
}

// Основной метод лексера: получить следующую лексему
LexemType Scanner::getNextLex(string& outLex)
{
    outLex.clear();

    // Пропускаем игнорируемые символы
    skipIgnored();

    // Конец текста
    char c = peek();
    if (c == '\0') return LexemType::T_END;

    // Идентификатор или ключевое слово
    if (isIdentStart(c))
    {
        string lex;
        lex.push_back(getChar());
        while (isIdentPart(peek())) lex.push_back(getChar());
        LexemType code = checkKeyword(lex);
        outLex = lex;

        if (code == LexemType::IDENT && lex.length() > MAX_NUMERIC_LITERAL_LEN) return LexemType::T_ERR;

        return code;
    }


    // Десятичные и шестнадацатеричные числа
    if (isDigit(c)) {
        char first = getChar();
        if (first == '0') {
            char nx = peek();
            if (nx == 'x' || nx == 'X')
            {
                // требуется >=1 16-ричная цифра
                getChar(); // убираем x/X
                string lex = "0";
                lex.push_back(nx);
                if (!isHexDigit(peek()))
                {
                    // ошибка: 0x без цифр
                    outLex = lex;
                    return LexemType::T_ERR;
                }
                while (isHexDigit(peek())) lex.push_back(getChar());
                outLex = lex;

                if (lex.length() > MAX_NUMERIC_LITERAL_LEN) return LexemType::T_ERR;

                return LexemType::HEX_LITERAL;
            }
            else if (isDigit(nx) || nx == '.')
            {
                // цифра после 0 - читаем как DEC
                string lex;
                lex.push_back(first); // '0'

                bool hasDot = false;
                bool hasE = false;
                bool hasSign = false;

                while (true)
                {
                    char c = peek();
                    if (c == '.')
                    {
                        if (hasDot || hasE) break;
                        hasDot = true;
                    }
                    else if ((c | 32) == 'e')
                    {
                        char nxt = peek(1);
                        if (nxt != '+' && nxt != '-' && !isDigit(nxt))
                            return LexemType::T_ERR;

                        if (hasE) break;
                        hasE = true;
                    }
                    else if (c == '+' || c == '-')
                    {
                        if (!isDigit(peek(1)))
                            return LexemType::T_ERR;

                        if (hasSign || !hasE) break;
                        hasSign = true;
                    }
                    else if (!isDigit(c)) break;
                    lex.push_back(getChar());
                }
                outLex = lex;

                if (lex.length() > MAX_NUMERIC_LITERAL_LEN) return LexemType::T_ERR;
                return (hasE || hasDot) ? LexemType::DOUBLE_LITERAL : LexemType::DEC_LITERAL;
            }
            else
            {
                outLex = "0";
                return LexemType::DEC_LITERAL;
            }
        }
        else
        {
            // первая цифра 1..9
            string lex;
            lex.push_back(first); // '1'/.../'9' - одна цифра
            bool hasDot = false;
            bool hasE = false;
            bool hasSign = false;

            while (true)
            {
                char c = peek();
                if (c == '.')
                {
                    if (hasDot || hasE) break;
                    hasDot = true;
                }
                else if ((c | 32) == 'e')
                {
                    char nxt = peek(1);
                    if (nxt != '+' && nxt != '-' && !isDigit(nxt))
                        return LexemType::T_ERR;

                    if (hasE) break;
                    hasE = true;
                }
                else if (c == '+' || c == '-')
                {
                    if (!isDigit(peek(1)))
                        return LexemType::T_ERR;

                    if (hasSign || !hasE) break;
                    hasSign = true;
                }
                else if (!isDigit(c)) break;
                lex.push_back(getChar());
            }
            outLex = lex;
            if (lex.length() > MAX_NUMERIC_LITERAL_LEN) return LexemType::T_ERR;
            return (hasE || hasDot) ? LexemType::DOUBLE_LITERAL : LexemType::DEC_LITERAL;
        }
    }

    // Операторы и разделители
    char n1 = peek(1);
    switch (c)
    {
    case '+': getChar(); outLex = "+"; return LexemType::PLUS;
    case '-': getChar(); outLex = "-"; return LexemType::MINUS;
    case '*': getChar(); outLex = "*"; return LexemType::MULT;
    case '%': getChar(); outLex = "%"; return LexemType::MOD;
    case ';': getChar(); outLex = ";"; return LexemType::SEMI;
    case ',': getChar(); outLex = ","; return LexemType::COMMA;
    case '(': getChar(); outLex = "("; return LexemType::LPAREN;
    case ')': getChar(); outLex = ")"; return LexemType::RPAREN;
    case '{': getChar(); outLex = "{"; return LexemType::LBRACE;
    case '}': getChar(); outLex = "}"; return LexemType::RBRACE;
    case ':': getChar(); outLex = ":"; return LexemType::COLON;
    case '=':
        getChar();
        if (peek() == '=') { getChar(); outLex = "=="; return LexemType::EQ; }
        outLex = "="; return LexemType::ASSIGN;
    case '!':
        getChar();
        if (peek() == '=') { getChar(); outLex = "!="; return LexemType::NEQ; }
        // одиночный '!' — лексическая ошибка :)
        outLex = "!"; return LexemType::T_ERR;
    case '<':
        getChar();
        if (peek() == '<') { getChar(); outLex = "<<"; return LexemType::SHL; }
        if (peek() == '=') { getChar(); outLex = "<="; return LexemType::LE; }
        outLex = "<"; return LexemType::LT;
    case '>':
        getChar();
        if (peek() == '>') { getChar(); outLex = ">>"; return LexemType::SHR; }
        if (peek() == '=') { getChar(); outLex = ">="; return LexemType::GE; }
        outLex = ">"; return LexemType::GT;
    case '/':
        getChar();
        outLex = "/";
        return LexemType::DIV;
    case '&':
        getChar();
        if (peek() == '&')
        {
            getChar();
            outLex = "&&";
            return LexemType::LAND;
        }
        outLex = "&";
        return LexemType::T_ERR;

    case '|':
        getChar();
        if (peek() == '|')
        {
            getChar();
            outLex = "||";
            return LexemType::LOR;
        }
        outLex = "|";
        return LexemType::T_ERR;
    default:
        // неизвестный/недопустимый символ
        {
            string s;
            s.push_back(getChar());
            outLex = s;
            return LexemType::T_ERR;
        }
    }
}

pair<int, int> Scanner::getLineCol() const {
    int line = 1;
    int col = 0;
    size_t pos = 0;
    while (pos < currentPos && pos < text.size())
    {
        char c = text[pos];
        if (c == '\n')
        {
            line++;
            col = 1;
        }
        else col++;
        pos++;
    }
    return { line, col };
}

size_t Scanner::getPos() const
{
    return currentPos;
}

void Scanner::setPos(size_t pos)
{
    currentPos = pos;
}

void Scanner::loadFromString(const string& source)
{
    text = source;
    text.push_back('\0');
    currentPos = 0;
}
