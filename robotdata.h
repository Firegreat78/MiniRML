// RobotData.h
#ifndef ROBOTDATA_H
#define ROBOTDATA_H

#include <cinttypes>
#include <array>
#include <vector>

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

    static constexpr double minSegmentLength = 10.0;
    static constexpr double maxSegmentLength = 10.0 * minSegmentLength;
    static std::array<double, maxSegmentAmount> segmentLengths;

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
    void setX(uint8_t, double);
    void setY(uint8_t, double);
    void setZ(uint8_t, double);

    // поворот
    void rotate(uint8_t, double, double, double, double);

    bool isJointInsideWorkspace(uint8_t) const;
};

#endif // !ROBOTDATA_H
