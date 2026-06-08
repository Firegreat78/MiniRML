// RobotData.h
#ifndef ROBOTDATA_H
#define ROBOTDATA_H

#include <cinttypes>
#include <array>
#include <vector>
#include <optional>

using Point = std::array<double, 3>;

// данные о шарнирах робота
class RobotData
{
private:
    std::vector<Point> points;

    RobotData();
    ~RobotData();
public:
    static constexpr std::uint8_t minSegmentAmount = 3;
    static constexpr std::uint8_t maxSegmentAmount = 16;
    static uint8_t segmentAmount;

    static constexpr Point::value_type minSegmentLength = 10.0;
    static constexpr Point::value_type maxSegmentLength = 10.0 * minSegmentLength;
    static std::array<Point::value_type, maxSegmentAmount> segmentLengths;

    static constexpr double minWorkspaceSize = 40.0;
    static constexpr double maxWorkspaceSize = 10.0 * minWorkspaceSize;
    static double workspaceSize;

    RobotData(const RobotData&) = delete;
    RobotData& operator=(RobotData&) = delete;

    static RobotData& getInstance();

    void reset();

    // геттеры
    double getX(uint8_t) const;
    double getY(uint8_t) const;
    double getZ(uint8_t) const;

    // сеттеры
    void setX(uint8_t, Point::value_type);
    void setY(uint8_t, Point::value_type);
    void setZ(uint8_t, Point::value_type);

    // поворот
    void rotate(uint8_t, double, double, double, double);

    // вернуть координату, если шарнир выйдет за пределы при повороте
    std::optional<Point> getFirstWorkspaceHitCoord(uint8_t, uint8_t, double, double, double, double) const;

    bool isJointInsideWorkspace(uint8_t) const;
};

#endif // !ROBOTDATA_H
