// robotviewwidget.cpp
#include "RobotViewWidget.h"
#include "RobotData.h"

#include <array>
#include <vector>

#include <QPainter>

using namespace std;

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
    highlightedJoint = numeric_limits<decltype(RobotData::segmentAmount)>::max();
}

void RobotViewWidget::setCurrentPlane(ViewPlane plane)
{
    this->currentPlane = plane;
}

void RobotViewWidget::setInstrumentEnabled(bool instrumentEnabled)
{
    this->instrumentEnabled = instrumentEnabled;
}

void RobotViewWidget::setHighlightedJoint(decltype(RobotData::segmentAmount) jointIndex)
{
    this->highlightedJoint = jointIndex;
}

decltype(RobotData::segmentAmount) RobotViewWidget::getHighlightedJoint() const
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

    auto& rd = RobotData::getInstance();

    painter.setPen(QPen(Qt::lightGray, 1));
    painter.drawLine(0, centerY, width(), centerY);
    painter.drawLine(centerX, 0, centerX, height());

    double const step = RobotData::workspaceSize / tickAmount;

    painter.setFont(QFont("Segoe Print", 8));
    painter.setPen(QPen(Qt::gray, 1));

    for (int i = -tickAmount; i <= tickAmount; i++)
    {
        double coordValue = i * step;
        if (i == 0 || std::abs(coordValue) > maxCoord) continue;

        int screenX = centerX + static_cast<int>((coordValue / maxCoord) * centerX);

        if (screenX < 0 || screenX > width()) continue;

        painter.drawLine(screenX, centerY - tickSize, screenX, centerY + tickSize);

        QString label = QString::number(coordValue, 'f', 1);
        painter.drawText(screenX - 15, centerY - 8, 30, 20,
                         Qt::AlignCenter, label);
    }

    for (int i = -tickAmount; i <= tickAmount; i++)
    {
        double coordValue = i * step;
        if (i == 0 || std::abs(coordValue) > maxCoord) continue;

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

        // Draw filled circle
        painter.setBrush(QBrush(c));
        qreal sz = i == highlightedJoint ? jointSize * 1.8  : jointSize;
        painter.drawEllipse(p, sz, sz);

        if (i != RobotData::segmentAmount || !instrumentEnabled) continue;

        painter.setBrush(Qt::black);
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(p, jointSize / 2, jointSize / 2);
    }
}

void RobotViewWidget::drawPerpendiculars(QPainter& painter)
{
    if (highlightedJoint > RobotData::segmentAmount)
        return;

    auto& rd = RobotData::getInstance();

    // позиция выделенного шарнира
    QPointF jointPos = project(
        rd.getX(highlightedJoint),
        rd.getY(highlightedJoint),
        rd.getZ(highlightedJoint)
        );

    int centerX = width() / 2;
    int centerY = height() / 2;

    QPointF perpToX(jointPos.x(), centerY);
    QPointF perpToY(centerX, jointPos.y());

    painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));

    painter.drawLine(jointPos, perpToX);
    painter.drawLine(jointPos, perpToY);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::yellow, 2));
    painter.drawEllipse(perpToX, 4, 4);
    painter.drawEllipse(perpToY, 4, 4);

    painter.setPen(QPen(Qt::white, 1));
    painter.setFont(QFont("Segoe Print", 8));

    QString xCoord = QString::number(rd.getX(highlightedJoint), 'f', 2);
    QString yCoord = QString::number(rd.getY(highlightedJoint), 'f', 2);
    QString zCoord = QString::number(rd.getZ(highlightedJoint), 'f', 2);

    painter.drawText(perpToX.x() + 5, perpToX.y() - 5,
        tr("%1: %2")
            .arg(currentPlane == ViewPlane::YOZ ? "Y" : "Z")
            .arg(currentPlane == ViewPlane::YOZ ? yCoord : xCoord)
    );

    painter.drawText(perpToY.x() + 5, perpToY.y() - 5,
        tr("%1: %2")
            .arg(currentPlane == ViewPlane::XOY ? "Y" : "Z")
            .arg(currentPlane == ViewPlane::XOY ? yCoord : zCoord)
    );
}
