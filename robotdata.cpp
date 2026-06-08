// robotdata.cpp
#include "RobotData.h"

#include <cmath>
#include <optional>
#include <cmath>
#include <algorithm>

using namespace std;

uint8_t RobotData::segmentAmount = minSegmentAmount;
double RobotData::workspaceSize = (RobotData::maxWorkspaceSize + RobotData::minWorkspaceSize) / 2.0;

array<double, RobotData::maxSegmentAmount> RobotData::segmentLengths = []{
    array<double, RobotData::maxSegmentAmount> arr;
    arr.fill(RobotData::minSegmentLength);
    return arr;
}();

using Point = std::array<double, 3>;

constexpr double pi = 3.1415926535897932384626433832795;

std::array<double, 6> getMinMaxCoordinatesOnRotation(
    Point const& initial,
    Point const& pivot,
    double normalX,
    double normalY,
    double normalZ,
    double radians)
{

    if (radians < 0.0)
    {
        normalX = -normalX;
        normalY = -normalY;
        normalZ = -normalZ;
        radians = -radians;
    }

    if (radians > 2 * pi) radians = 2 * pi;

    const double len = std::sqrt(
        normalX * normalX +
        normalY * normalY +
        normalZ * normalZ);

    const double nx = normalX / len;
    const double ny = normalY / len;
    const double nz = normalZ / len;

    const double rx = initial[0] - pivot[0];
    const double ry = initial[1] - pivot[1];
    const double rz = initial[2] - pivot[2];

    // parallel = (r·n)n
    const double dot =
        rx * nx +
        ry * ny +
        rz * nz;

    // центр окружности вращения
    const Point center =
        {
            pivot[0] + dot * nx,
            pivot[1] + dot * ny,
            pivot[2] + dot * nz
        };

    // u = r - parallel
    const Point u =
        {
            rx - dot * nx,
            ry - dot * ny,
            rz - dot * nz
        };

    // v = n × u
    const Point v =
        {
            ny * u[2] - nz * u[1],
            nz * u[0] - nx * u[2],
            nx * u[1] - ny * u[0]
        };

    auto coordinateAt = [&](std::size_t axis, double phi)
    {
        return center[axis]
               + u[axis] * std::cos(phi)
               + v[axis] * std::sin(phi);
    };

    std::array<double, 6> result;

    for (std::size_t axis = 0; axis < 3; ++axis)
    {
        const double startValue = initial[axis];
        const double endValue = coordinateAt(axis, radians);

        double minValue = min(startValue, endValue);
        double maxValue = max(startValue, endValue);

        const double amplitude = std::sqrt(u[axis] * u[axis] + v[axis] * v[axis]);

        if (amplitude >= 1e-12)
        {
            double phiMax = std::atan2(v[axis], u[axis]);

            if (phiMax < 0.0)
                phiMax += 2.0 * pi;

            if (phiMax <= radians)
                maxValue = max(maxValue, center[axis] + amplitude);

            double phiMin = phiMax + pi;

            if (phiMin >= 2.0 * pi) phiMin -= 2.0 * pi;

            if (phiMin <= radians)
                minValue = min(minValue, center[axis] - amplitude);
        }

        result[2 * axis] = minValue;
        result[2 * axis + 1] = maxValue;
    }

    return result;
}

// точка вращается по окружности вокруг оси. Функция определяет точку, когда во время вращения она впервые выйдет за пределы
std::optional<Point> getBoundaryFirstHit(
    Point const& initial,
    Point const& pivot,
    double normalX,
    double normalY,
    double normalZ,
    double radians,
    double minX, double maxX,
    double minY, double maxY,
    double minZ, double maxZ)
{
    static constexpr double EPS = 1e-12;

    if (radians < 0.0)
    {
        normalX = -normalX;
        normalY = -normalY;
        normalZ = -normalZ;
        radians = -radians;
    }

    if (radians > 2.0 * pi)
        radians = 2.0 * pi;

    double const len =
        std::sqrt(
            normalX * normalX +
            normalY * normalY +
            normalZ * normalZ);

    if (len < EPS)
        return std::nullopt;

    double const nx = normalX / len;
    double const ny = normalY / len;
    double const nz = normalZ / len;

    //----------------------------------
    // параметризация окружности
    //----------------------------------

    double const rx = initial[0] - pivot[0];
    double const ry = initial[1] - pivot[1];
    double const rz = initial[2] - pivot[2];

    double const dot =
        rx * nx +
        ry * ny +
        rz * nz;

    double const cx = pivot[0] + dot * nx;
    double const cy = pivot[1] + dot * ny;
    double const cz = pivot[2] + dot * nz;

    double const ux = rx - dot * nx;
    double const uy = ry - dot * ny;
    double const uz = rz - dot * nz;

    double const vx = ny * uz - nz * uy;
    double const vy = nz * ux - nx * uz;
    double const vz = nx * uy - ny * ux;

    auto coord =
        [](double A,
           double B,
           double C,
           double phi)
    {
        return A +
               B * std::cos(phi) +
               C * std::sin(phi);
    };

    auto pos =
        [&](double phi)
    {
        return Point
            {
                coord(cx, ux, vx, phi),
                coord(cy, uy, vy, phi),
                coord(cz, uz, vz, phi)
            };
    };

    auto inside =
        [&](double phi)
    {
        double const x = coord(cx, ux, vx, phi);
        double const y = coord(cy, uy, vy, phi);
        double const z = coord(cz, uz, vz, phi);

        return
            x >= minX && x <= maxX &&
            y >= minY && y <= maxY &&
            z >= minZ && z <= maxZ;
    };

    if (!inside(0.0))
        return std::nullopt;

    //----------------------------------
    // Collect all boundary crossings
    //----------------------------------

    std::vector<double> events;

    auto addPlaneRoots =
        [&](double A,
            double B,
            double C,
            double limit)
    {
        double const R =
            std::sqrt(B * B + C * C);

        if (R < EPS)
            return;

        double const k =
            (limit - A) / R;

        if (k < -1.0 || k > 1.0)
            return;

        double const delta =
            std::atan2(C, B);

        double const alpha =
            std::acos(k);

        auto addRoot =
            [&](double phi)
        {
            while (phi < 0.0)
                phi += 2.0 * pi;

            while (phi >= 2.0 * pi)
                phi -= 2.0 * pi;

            if (phi > EPS &&
                phi <= radians + EPS)
            {
                events.push_back(phi);
            }
        };

        addRoot(delta + alpha);
        addRoot(delta - alpha);
    };

    // X
    addPlaneRoots(cx, ux, vx, minX);
    addPlaneRoots(cx, ux, vx, maxX);

    // Y
    addPlaneRoots(cy, uy, vy, minY);
    addPlaneRoots(cy, uy, vy, maxY);

    // Z
    addPlaneRoots(cz, uz, vz, minZ);
    addPlaneRoots(cz, uz, vz, maxZ);

    if (events.empty())
        return std::nullopt;

    //----------------------------------
    // сортируем и удаляем дубликаты
    //----------------------------------

    std::sort(events.begin(), events.end());

    events.erase(
        std::unique(
            events.begin(),
            events.end(),
            [](double a, double b)
            {
                return std::abs(a - b) < 1e-10;
            }),
        events.end());

    //----------------------------------
    // находим первое пересечение изнутри -> наружу
    //----------------------------------

    double const epsPhi = 1e-9;

    for (double const phi : events)
    {
        double before =
            std::max(0.0, phi - epsPhi);

        double after =
            std::min(radians, phi + epsPhi);

        bool const wasInside =
            inside(before);

        bool const nowInside =
            inside(after);

        if (wasInside && !nowInside)
        {
            return pos(phi);
        }
    }

    return std::nullopt;
}

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
        offset += segmentLengths.at(i);
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

void RobotData::rotate(
    uint8_t pointIdx,
    double normalX,
    double normalY,
    double normalZ,
    double radians)
{
    for (uint8_t i = pointIdx + 1; i < points.size(); i++)
    {
        Point p = rotatePointAroundAxis(
            points[i],
            points[pointIdx],
            normalX, normalY, normalZ,
            radians);

        // обновляем точку
        points.at(i)[0] = p[0];
        points.at(i)[1] = p[1];
        points.at(i)[2] = p[2];
    }
}

void RobotData::reset()
{
    points.clear();

    if (points.capacity() <= RobotData::segmentAmount)
        points.reserve(RobotData::segmentAmount + 1);

    Point::value_type offset = 0.0;
    for (uint8_t i = 0; i < segmentAmount; i++)
    {
        points.push_back({offset, 0, 0});
        offset += segmentLengths.at(i);
    }
    points.push_back({offset, 0, 0});
}

bool RobotData::isJointInsideWorkspace(uint8_t jointIndex) const
{
    double const x = getX(jointIndex);
    double const y = getY(jointIndex);
    double const z = getZ(jointIndex);

    return abs(x * 2.0) <= workspaceSize &&
           abs(y * 2.0) <= workspaceSize &&
           abs(z * 2.0) <= workspaceSize;
}

optional<Point> RobotData::getFirstWorkspaceHitCoord(
    uint8_t pointIdx,
    uint8_t rotatedJointIdx,
    double normalX,
    double normalY,
    double normalZ,
    double radians) const
{
    auto hitpoint = getBoundaryFirstHit(
        points.at(rotatedJointIdx),
        points.at(pointIdx),
        normalX, normalY, normalZ,
        radians,
        -RobotData::workspaceSize/2, RobotData::workspaceSize/2,
        -RobotData::workspaceSize/2, RobotData::workspaceSize/2,
        -RobotData::workspaceSize/2, RobotData::workspaceSize/2
    );

    if (!hitpoint.has_value()) return nullopt;
    return hitpoint;
}
