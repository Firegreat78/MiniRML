#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QPointer>
#include <QCloseEvent>
#include <QShortcut>
#include <QList>
#include <QListWidgetItem>

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
    QList<QPointer<QAction>> colorActions;
    QPointer<QShortcut> enterShortcutNextStepButton = nullptr;
    std::array<QPointer<QAction>, RobotData::maxSegmentAmount> updateSegmentLengthsActions;
    void setupUi();

    QPointer<RobotViewWidget> robotViewWidget = nullptr;

    Scanner* sc = nullptr;
    QPointer<Diagram> dg = nullptr;
    std::map<std::string, SemNode> variables;
    QPointer<QThread> parserThrd = nullptr;

    void addColor(QColor);
    void deleteColor(uint8_t);

    void redrawRobot();

signals:
    void updatedActionPlane(RobotViewWidget::ViewPlane);

private slots:
    void onUpdateSegmentAmount();
    void onUpdateSegmentLength();
    void onUpdatedActionPlane(RobotViewWidget::ViewPlane);
    void onLoadedSrcCode();
    void onUpdateWorkspaceSize();
    void onParseError(const QString&);
    void onParseFinished();
    void cleanupParser();
    void onAddColor();
    void onDeleteColor();
    void onEditColor();
    void onNextStep();
    void onRobotDataItemClicked(QListWidgetItem*);
    void onChangeTickAmountRequested();
    void onChangeTickSizeRequested();
    void onChangeSegmentThicknessRequested();
    void onChangeJointSizeRequested();

public slots:
    void onVariablesReceived(std::vector<std::pair<std::string, SemNode*>> const&);
    void onRobotDataShouldUpdate();
    void onRecvCurrentStepMsg(QString const&);
protected:
    void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
