// robotdata.cpp
#include "RobotData.h"

#include <cmath>

using namespace std;

uint8_t RobotData::segmentAmount = minSegmentAmount;

array<double, RobotData::maxSegmentAmount> RobotData::segmentLengths = []{
    array<double, RobotData::maxSegmentAmount> arr;
    arr.fill(RobotData::minSegmentLength);
    return arr;
}();

Point rotatePointAroundAxis(
    Point const& initial,
    Point const& pivot,
    double normalX,
    double normalY,
    double normalZ,
    double radians
    )
{
    // Вектор от pivot до initial
    double x = initial[0] - pivot[0];
    double y = initial[1] - pivot[1];
    double z = initial[2] - pivot[2];

    // Нормализация оси вращения
    double len = std::sqrt(
        normalX * normalX +
        normalY * normalY +
        normalZ * normalZ
        );

    // len всегда будет не близкое к нулю число, это проверяется при интерпретации
    double ux = normalX / len;
    double uy = normalY / len;
    double uz = normalZ / len;

    double cosA = cos(radians);
    double sinA = sin(radians);

    // Формула Родрига
    double rotatedX =
        x * cosA +
        (uy * z - uz * y) * sinA +
        ux * (ux * x + uy * y + uz * z) * (1.0 - cosA);

    double rotatedY =
        y * cosA +
        (uz * x - ux * z) * sinA +
        uy * (ux * x + uy * y + uz * z) * (1.0 - cosA);

    double rotatedZ =
        z * cosA +
        (ux * y - uy * x) * sinA +
        uz * (ux * x + uy * y + uz * z) * (1.0 - cosA);

    // Возвращаем обратно относительно pivot
    return {
        rotatedX + pivot[0],
        rotatedY + pivot[1],
        rotatedZ + pivot[2],
    };
}

RobotData& RobotData::getInstance()
{
    static RobotData obj;
    return obj;
}

RobotData::RobotData()
{
    double offset = 0.0;
    for (uint8_t i = 0; i < segmentAmount; i++)
    {
        points.push_back({offset, 0, 0});
        offset += segmentLengths[i];
    }
    points.push_back({offset, 0, 0});
}

RobotData::~RobotData() {}

double RobotData::getX(uint8_t const pointIdx) const {
    return points.at(pointIdx)[0];
}

double RobotData::getY(uint8_t const pointIdx) const {
    return points.at(pointIdx)[1];
}

double RobotData::getZ(uint8_t const pointIdx) const {
    return points.at(pointIdx)[2];
}

void RobotData::setX(uint8_t const pointIdx, double value) {
    points.at(pointIdx)[0] = value;
}

void RobotData::setY(uint8_t const pointIdx, double value) {
    points.at(pointIdx)[1] = value;
}

void RobotData::setZ(uint8_t const pointIdx, double value) {
    points.at(pointIdx)[2] = value;
}

void RobotData::rotate(uint8_t pointIdx, double normalX, double normalY, double normalZ, double radians)
{
    for (uint8_t i = pointIdx + 1; i < points.size(); i++)
    {
        Point p = rotatePointAroundAxis(
            points[i],
            points[pointIdx],
            normalX, normalY, normalZ,
            radians);

        // обновляем точку
        points[i][0] = p[0];
        points[i][1] = p[1];
        points[i][2] = p[2];
    }
}

void RobotData::reset()
{
    points.clear();

    if (points.capacity() <= RobotData::segmentAmount)
        points.reserve(RobotData::segmentAmount + 1);

    double offset = 0.0;
    for (uint8_t i = 0; i < segmentAmount; i++)
    {
        points.push_back({offset, 0, 0});
        offset += segmentLengths[i];
    }
    points.push_back({offset, 0, 0});
}
