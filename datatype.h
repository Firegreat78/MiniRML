// DataType.h
#ifndef DATA_TYPE_H
#define DATA_TYPE_H

enum class DATA_TYPE
{
    TYPE_INT = 1, // целый []
    TYPE_SHORT_INT, // целый short []
    TYPE_LONG_INT, // целый long []
    TYPE_DOUBLE, // с плавающей запятой двойной точности
    //TYPE_FIXED_POINT, // с фиксированной запятой формата Q15.16 (32 бита)
    TYPE_BOOL, // булева
    TYPE_FUNCT, // функция
    TYPE_SCOPE // составной оператор
};

#endif // DATA_TYPE_H
