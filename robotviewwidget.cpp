// robotviewwidget.cpp
#include "RobotViewWidget.h"
#include "RobotData.h"

#include <array>
#include <vector>

#include <QPainter>

using namespace std;

QList<QColor> RobotViewWidget::colors = {};

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
}

void RobotViewWidget::setCurrentPlane(ViewPlane plane)
{
    this->currentPlane = plane;
}

void RobotViewWidget::setInstrumentEnabled(bool instrumentEnabled)
{
    this->instrumentEnabled = instrumentEnabled;
}

void RobotViewWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);

    painter.setRenderHint(QPainter::Antialiasing);

    int centerX = width() / 2;
    int centerY = height() / 2;

    auto project = [&](double x, double y, double z)
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
            centerX + px * scale,
            centerY + py * scale
        );
    };

    auto& rd = RobotData::getInstance();

    // оси
    painter.setPen(QPen(Qt::lightGray, 1));

    painter.drawLine(0, centerY, width(), centerY);
    painter.drawLine(centerX, 0, centerX, height());

    // робот
    painter.setPen(QPen(Qt::white, 3));

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

    // суставы
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        int const sz = 8;
        QPointF p = project(
            rd.getX(i),
            rd.getY(i),
            rd.getZ(i)
            );
        painter.setBrush(colors.empty() ? Qt::white : colors[i % colors.size()]);
        painter.setPen(colors.empty() ? Qt::white : colors[i % colors.size()]);
        painter.drawEllipse(p, sz, sz);

        if (i != RobotData::segmentAmount || !instrumentEnabled) continue;

        painter.setBrush(Qt::black);
        painter.setPen(Qt::white);
        painter.drawEllipse(p, sz / 2, sz / 2);
    }
}
