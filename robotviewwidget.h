#ifndef ROBOTVIEWWIDGET_H
#define ROBOTVIEWWIDGET_H

#include <QWidget>
#include <QList>

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
    void setHighlightedJoint(uint8_t);

    uint8_t getHighlightedJoint() const;

    static constexpr uint8_t maxColorAmount = 12;
    static QList<QColor> colors;
    static QColor getColor(uint8_t);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ViewPlane currentPlane;
    bool instrumentEnabled;

    int widgetSize;
    double maxCoord;
    double scale;

    uint8_t highlightedJoint;
};

#endif
