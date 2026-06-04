// Scanner.h
#ifndef SCANNER_H
#define SCANNER_H

#include <string>
#include "lexemtype.h"

class Scanner {
public:
    Scanner();

    // Загрузить файл; вернуть true при успехе
    bool loadFile(const std::string& fileName);
    // Загрузить исходный текст из строки
    void loadFromString(const std::string& source);

    // Получить следующую лексему; лексема записывается в outLex
    // Возвращает её код
    LexemType getNextLex(std::string&);

    std::pair<int, int> getLineCol() const;

    // возвращает текущую позицию/индекс в внутреннем буфере/строке
    size_t getPos() const;

    // устанавливает позицию — после этого следующий вызов чтения будет с этой позиции
    void setPos(size_t pos);

private:
    std::string text; // исходный текст + завершающий '\0'
    std::size_t currentPos; // текущая позиция в text

    char peek(std::size_t offset = 0) const; // получить текущий символ, но не сдвигать позицию
    char getChar(); // получить текущий символ и сдвинуть позицию
    void ungetChar(); // "вернуть назад" последний полученный символ (сдвинуть позицию влево)

    // классификаторы символов (буква, цифра, начало идентификатора и т.п.)
    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isIdentStart(char c);
    static bool isIdentPart(char c);

    void skipIgnored(); // проверить пробелы и комментарии
    LexemType checkKeyword(const std::string&); // проверить ключевые слова
};

#endif // SCANNER_H
