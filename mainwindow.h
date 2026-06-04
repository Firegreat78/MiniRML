#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QPointer>
#include <QCloseEvent>
#include <QShortcut>

#include <map>

#include "semnode.h"
#include "scanner.h"
#include "diagram.h"
#include "robotviewwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QPointer<RobotViewWidget> robotViewWidget = nullptr;

    Scanner* sc = nullptr;
    QPointer<Diagram> dg = nullptr;
    std::map<std::string, SemNode> variables; // todo: del?
    QPointer<QThread> parserThrd = nullptr;
    std::array<QAction*, RobotData::maxSegmentAmount> updateSegmentLengthsActions;

    QPointer<QShortcut> enterShortcutNextStepButton = nullptr;

    void redrawRobot();

signals:
    void updateSegmentLength(uint8_t);
    void updatedActionPlane(RobotViewWidget::ViewPlane);

private slots:
    void onUpdateSegmentAmount();
    void onUpdateSegmentLength(uint8_t);
    void onUpdatedActionPlane(RobotViewWidget::ViewPlane);
    void onLoadedSrcCode();
    void onUpdateWorkspaceSize();
    void onParseError(const QString&);
    void onParseFinished();
    void cleanupParser();

public slots:
    void onVariablesReceived(std::vector<std::pair<std::string, SemNode*>> const&);
    void onRobotDataShouldUpdate();
    void onRecvCurrentStepMsg(QString const&);
protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
