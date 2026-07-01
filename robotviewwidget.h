// robotviewwidget.h
#ifndef ROBOTVIEWWIDGET_H
#define ROBOTVIEWWIDGET_H

#include <QWidget>
#include <QList>

#include "robotdata.h"

class RobotViewWidget : public QWidget
{
    Q_OBJECT

public:
    enum class ViewPlane : uint8_t
    {
        XOY,
        YOZ,
        XOZ
    };

    explicit RobotViewWidget(
        int sideSize,
        double maxCoordAbs,
        QWidget* parent = nullptr
        );

    void setCurrentPlane(ViewPlane);
    void setInstrumentEnabled(bool);
    void setHighlightedJoint(decltype(RobotData::segmentAmount));

    decltype(RobotData::segmentAmount) getHighlightedJoint() const;

    static constexpr uint8_t maxColorAmount = 12;
    static QList<QColor> colors;
    static QColor getColor(uint8_t);

    static constexpr uint8_t minTickAmount = 4;
    static constexpr uint8_t maxTickAmount = 12;
    static uint8_t tickAmount;

    static constexpr uint8_t minTickSize = 4;
    static constexpr uint8_t maxTickSize = 10;
    static uint8_t tickSize;

    static constexpr qreal minJointSize = 4.0;
    static constexpr qreal maxJointSize = 10.0;
    static qreal jointSize;

    static constexpr uint8_t minSegmentThickness = 1;
    static constexpr uint8_t maxSegmentThickness = 4;
    static uint8_t segmentThickness;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ViewPlane currentPlane;
    bool instrumentEnabled;

    int widgetSize;
    double maxCoord;
    double scale;

    decltype(RobotData::segmentAmount) highlightedJoint;

    void drawAxes(QPainter&);
    void drawRobot(QPainter&);
    void drawPerpendiculars(QPainter&);

    QPointF project(double, double, double) const;
};

#endif
