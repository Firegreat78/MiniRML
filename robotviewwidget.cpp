// robotviewwidget.cpp
#include "RobotViewWidget.h"
#include "RobotData.h"

#include <array>
#include <vector>

#include <QPainter>

using namespace std;

// подобрать такой цвет, чтобы было различим выбранный шарнир
QColor getDistinguishableColor(const QColor& baseColor)
{
    int h, s, l;
    baseColor.getHsl(&h, &s, &l);

    int newHue = (h + 180) % 360;

    int newLight = l > 128 ? l - 80 : l + 80;
    newLight = qBound(0, newLight, 255);

    return QColor::fromHsl(newHue, s, newLight);
}

QList<QColor> RobotViewWidget::colors = {};

uint8_t RobotViewWidget::tickAmount = (minTickAmount + maxTickAmount) / 2;
uint8_t RobotViewWidget::tickSize = (minTickSize + maxTickSize) / 2;
qreal RobotViewWidget::jointSize = (minJointSize + maxJointSize) / 2.0;
uint8_t RobotViewWidget::segmentThickness = (minSegmentThickness + maxSegmentThickness) / 2;

QColor RobotViewWidget::getColor(uint8_t index)
{
    return colors.empty() ? Qt::white : colors[index % colors.size()];
}

RobotViewWidget::RobotViewWidget(
    int sideSize,
    double maxCoordAbs,
    QWidget* parent
    )
    : QWidget(parent),
    currentPlane(ViewPlane::YOZ),
    widgetSize(sideSize),
    maxCoord(maxCoordAbs)
{
    setFixedSize(widgetSize, widgetSize);
    scale = widgetSize / (2.0 * maxCoord);
    highlightedJoint = 0;
}

void RobotViewWidget::setCurrentPlane(ViewPlane plane)
{
    this->currentPlane = plane;
}

void RobotViewWidget::setInstrumentEnabled(bool instrumentEnabled)
{
    this->instrumentEnabled = instrumentEnabled;
}

void RobotViewWidget::setHighlightedJoint(uint8_t jointIndex)
{
    this->highlightedJoint = jointIndex;
}

uint8_t RobotViewWidget::getHighlightedJoint() const
{
    return this->highlightedJoint;
}

void RobotViewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);
    painter.setRenderHint(QPainter::Antialiasing);

    drawRobot(painter);
    drawPerpendiculars(painter);
    drawAxes(painter);
}

QPointF RobotViewWidget::project(double x, double y, double z) const
{
    double px = 0;
    double py = 0;

    switch (currentPlane)
    {
    case ViewPlane::XOY:
        px = x;
        py = -y;
        break;

    case ViewPlane::YOZ:
        px = z;
        py = -y;
        break;

    case ViewPlane::XOZ:
        px = x;
        py = -z;
        break;
    }

    return QPointF(
        width() / 2 + px * scale,
        height() / 2 + py * scale
        );
}

void RobotViewWidget::drawAxes(QPainter& painter)
{
    int const centerX = width() / 2;
    int const centerY = height() / 2;

    // Get workspace range
    auto& rd = RobotData::getInstance();

    // Draw axes lines
    painter.setPen(QPen(Qt::lightGray, 1));
    painter.drawLine(0, centerY, width(), centerY);
    painter.drawLine(centerX, 0, centerX, height());

    // Calculate tick spacing
    double const step = RobotData::workspaceSize / tickAmount;

    // Set font for tick labels
    painter.setFont(QFont("Segoe Print", 8));
    painter.setPen(QPen(Qt::gray, 1));

    for (int i = -tickAmount; i <= tickAmount; i++)
    {
        double coordValue = i * step;
        if (std::abs(coordValue) > maxCoord) continue;

        int screenX = centerX + static_cast<int>((coordValue / maxCoord) * centerX);

        // Skip if outside widget bounds
        if (screenX < 0 || screenX > width()) continue;

        // Draw tick mark
        painter.drawLine(screenX, centerY - tickSize, screenX, centerY + tickSize);

        // Draw tick label
        QString label = QString::number(coordValue, 'f', 1);
        painter.drawText(screenX - 15, centerY - 8, 30, 20,
                         Qt::AlignCenter, label);
    }

    // Draw Y-axis ticks (vertical)
    for (int i = -tickAmount; i <= tickAmount; i++)
    {
        double coordValue = i * step;
        if (std::abs(coordValue) > maxCoord) continue;

        int screenY = centerY - static_cast<int>((coordValue / maxCoord) * centerY);

        if (screenY < 0 || screenY > height()) continue;

        painter.drawLine(centerX - tickSize, screenY, centerX + tickSize, screenY);

        QString label = QString::number(coordValue, 'f', 1);
        painter.drawText(centerX + 8, screenY - 8, 40, 20,
                         Qt::AlignLeft | Qt::AlignVCenter, label);
    }
}

void RobotViewWidget::drawRobot(QPainter& painter)
{
    auto& rd = RobotData::getInstance();

    // соединения между шарнирами
    painter.setPen(QPen(Qt::white, segmentThickness));
    for (uint8_t i = 0; i < RobotData::segmentAmount; i++)
    {
        QPointF p1 = project(
            rd.getX(i),
            rd.getY(i),
            rd.getZ(i)
            );

        QPointF p2 = project(
            rd.getX(i + 1),
            rd.getY(i + 1),
            rd.getZ(i + 1)
            );

        painter.drawLine(p1, p2);
    }

    // шарниры
    painter.setPen(Qt::NoPen);
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        QPointF p = project(
            rd.getX(i),
            rd.getY(i),
            rd.getZ(i)
            );
        QColor c(getColor(i));

        if (i == highlightedJoint)
        {
            qreal const outlineSize = jointSize*2.0;
            painter.setBrush(QBrush(getDistinguishableColor(c)));
            painter.drawEllipse(p, outlineSize, outlineSize);
        }

        // Draw filled circle
        painter.setBrush(QBrush(c));
        painter.drawEllipse(p, jointSize, jointSize);

        if (i != RobotData::segmentAmount || !instrumentEnabled) continue;

        painter.setBrush(Qt::black);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(p, jointSize / 2, jointSize / 2);
    }
}

void RobotViewWidget::drawPerpendiculars(QPainter& painter)
{
    if (highlightedJoint == numeric_limits<uint8_t>::max())
        return;

    auto& rd = RobotData::getInstance();

    // Get selected joint position
    QPointF jointPos = project(
        rd.getX(highlightedJoint),
        rd.getY(highlightedJoint),
        rd.getZ(highlightedJoint)
        );

    int centerX = width() / 2;
    int centerY = height() / 2;

    // Calculate perpendicular points on axes
    QPointF perpToX(jointPos.x(), centerY);  // Perpendicular to X-axis (vertical line to X-axis)
    QPointF perpToY(centerX, jointPos.y());   // Perpendicular to Y-axis (horizontal line to Y-axis)

    // Set pen style for perpendiculars
    painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));

    // Draw perpendicular to X-axis (vertical line)
    painter.drawLine(jointPos, perpToX);

    // Draw perpendicular to Y-axis (horizontal line)
    painter.drawLine(jointPos, perpToY);

    // Optional: Draw small circles at intersection points
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::yellow, 2));
    painter.drawEllipse(perpToX, 4, 4);
    painter.drawEllipse(perpToY, 4, 4);

    // Optional: Add coordinate labels
    painter.setPen(QPen(Qt::white, 1));
    painter.setFont(QFont("Segoe Print", 8));

    QString xCoord = QString::number(rd.getX(highlightedJoint), 'f', 2);
    QString yCoord = QString::number(rd.getY(highlightedJoint), 'f', 2);
    QString zCoord = QString::number(rd.getZ(highlightedJoint), 'f', 2);

    painter.drawText(perpToX.x() + 5, perpToX.y() - 5,
                     QString("%1: %2").arg(currentPlane == ViewPlane::XOY || currentPlane == ViewPlane::XOZ ? "X" : "Y")
                         .arg(currentPlane == ViewPlane::XOY || currentPlane == ViewPlane::XOZ ? xCoord : yCoord));

    painter.drawText(perpToY.x() + 5, perpToY.y() - 5,
                     QString("%1: %2").arg(currentPlane == ViewPlane::XOY ? "Y" : "Z")
                         .arg(currentPlane == ViewPlane::XOY ? yCoord : zCoord));
}
