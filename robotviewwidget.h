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
        Top,   // XOY
        Side,  // YOZ
        Front  // XOZ
    };

    explicit RobotViewWidget(
        int sideSize,
        double maxCoordAbs,
        QWidget* parent = nullptr
        );

    void setCurrentPlane(ViewPlane);
    void setInstrumentEnabled(bool);

    static constexpr uint8_t maxColorAmount = 12;
    static QList<QColor> colors;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ViewPlane currentPlane;
    bool instrumentEnabled;

    int widgetSize;
    double maxCoord;
    double scale;
};

#endif
