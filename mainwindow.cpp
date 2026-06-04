// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "robotdata.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QMessageBox>

#include <iostream>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    updateSegmentLengthsActions = {
        ui->segmentLength1Action,
        ui->segmentLength2Action,
        ui->segmentLength3Action,
        ui->segmentLength4Action,
        ui->segmentLength5Action,
        ui->segmentLength6Action,
    };

    QString seg_amt_txt(QString("Указать количество сегментов робота (текущее количество = %1)")
                            .arg(RobotData::segmentAmount)
                        );
    ui->updateSegmentAmountAction->setText(seg_amt_txt);

    ui->workspaceSizeAction->setText(
        QString("Размер рабочей области = %1")
        .arg(RobotData::workspaceSize)
    );
    connect(
        ui->workspaceSizeAction,
        &QAction::triggered,
        this,
        &MainWindow::onUpdateWorkspaceSize
    );

    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++)
    {
        qsizetype i = it - updateSegmentLengthsActions.begin();
        (*it)->setText(QString("Длина сегмента #%1 = %2")
            .arg(i+1)
            .arg(RobotData::segmentLengths[i])
        );
        (*it)->setVisible(i < RobotData::segmentAmount);
    }

    sc = new Scanner();
    ui->projectionWidgetLabel->hide();

    // длины сегментов + инструмент
    connect(ui->segmentLength1Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(0); });
    connect(ui->segmentLength2Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(1); });
    connect(ui->segmentLength3Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(2); });
    connect(ui->segmentLength4Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(3); });
    connect(ui->segmentLength5Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(4); });
    connect(ui->segmentLength6Action, &QAction::triggered, this, [this]() { emit updateSegmentLength(5); });
    connect(this, &MainWindow::updateSegmentLength, this, &MainWindow::onUpdateSegmentLength);
    connect(ui->toggleInstrumentAction, &QAction::triggered, this, &MainWindow::redrawRobot);

    // смена проекции
    connect(ui->actionPlaneXOY, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::Top); });
    connect(ui->actionPlaneXOZ, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::Front); });
    connect(ui->actionPlaneYOZ, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::Side); });
    connect(this, &MainWindow::updatedActionPlane, this, &MainWindow::onUpdatedActionPlane);

    connect(ui->uploadSourceCodeAction, &QAction::triggered, this, &MainWindow::onLoadedSrcCode);

    connect(ui->deleteSourceCodeAction,
            &QAction::triggered,
            this,
            [this]()
            {
                cleanupParser();
            });

    connect(ui->updateSegmentAmountAction, &QAction::triggered, this, &MainWindow::onUpdateSegmentAmount);
    auto& rd = RobotData::getInstance();
    rd.reset();
}

MainWindow::~MainWindow()
{
    if (parserThrd && parserThrd->isRunning())
    {
        parserThrd->terminate();
        parserThrd->wait(400);
    }
    delete ui;
}

void MainWindow::cleanupParser()
{
    qDebug() << "Deleting parser";

    ui->uploadSourceCodeAction->setEnabled(true);
    ui->deleteSourceCodeAction->setEnabled(false);
    ui->updateSegmentAmountAction->setEnabled(true);
    ui->nextStepButton->setEnabled(false);
    ui->workspaceSizeAction->setEnabled(true);

    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++
    ) (*it)->setEnabled(true);

    ui->variablesListWidget->clear();
    ui->robotDataListWidget->clear();
    ui->commandsListWidget->clear();

    setWindowTitle("Инструментальная система");

    if (dg)
        dg->requestStop();


    if (parserThrd)
    {
        parserThrd->quit();
        parserThrd->wait();
        delete parserThrd;
        parserThrd = nullptr;
    }

    dg = nullptr;

    ui->projectionWidgetLabel->hide();

    if (robotViewWidget)
        robotViewWidget->deleteLater();

    qDebug() << "Parser deleted";
}

void MainWindow::onUpdateSegmentAmount()
{
    bool ok;
    int const segments = QInputDialog::getInt(
        this,
        "Изменить количество сегментов",
        QString("Укажите количество сегментов (%1-%2):")
            .arg(RobotData::minSegmentAmount)
            .arg(RobotData::maxSegmentAmount),
        RobotData::segmentAmount,
        RobotData::minSegmentAmount,
        RobotData::maxSegmentAmount,
        1,
        &ok);
    if (!ok) return;

    RobotData::segmentAmount = segments;
    QString seg_amt_txt(QString("Указать количество сегментов робота (текущее количество = %1)")
        .arg(segments)
    );
    ui->updateSegmentAmountAction->setText(seg_amt_txt);
    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++)
    {
        qsizetype i = it - updateSegmentLengthsActions.begin();
        (*it)->setVisible(i < segments);
    }
}
void MainWindow::onVariablesReceived(vector<pair<string, SemNode*>> const& variables)
{
    ui->variablesListWidget->clear();

    auto typeToString = [](DATA_TYPE type) -> QString
    {
        switch (type)
        {
        case DATA_TYPE::TYPE_SHORT_INT: return "short";
        case DATA_TYPE::TYPE_INT:       return "int";
        case DATA_TYPE::TYPE_LONG_INT:  return "long";
        case DATA_TYPE::TYPE_BOOL:      return "bool";
        case DATA_TYPE::TYPE_DOUBLE:    return "double";
        default:                        return "неизвестно";
        }
    };

    auto formatVar = [&](const string& name, const SemNode* node, const auto& value) {
        return QString("[%1:%2] %3 %4 = %5")
        .arg(node->line)
        .arg(node->col)
        .arg(typeToString(node->DataType))
        .arg(QString::fromStdString(name))
        .arg(value);
    };

    for (auto const& var : variables)
    {
        SemNode* node = var.second;
        if (!node->isInitialized)
        {
            ui->variablesListWidget->addItem(
                QString("[%1:%2] %3 %4 = [неинициализирована]")
                    .arg(node->line)
                    .arg(node->col)
                    .arg(typeToString(node->DataType))
                    .arg(QString::fromStdString(var.first)));
            continue;
        }

        switch (node->DataType)
        {
        case DATA_TYPE::TYPE_SHORT_INT:
            ui->variablesListWidget->addItem(formatVar(var.first, node, node->Value.v_int16)); break;
        case DATA_TYPE::TYPE_INT:
            ui->variablesListWidget->addItem(formatVar(var.first, node, node->Value.v_int32)); break;
        case DATA_TYPE::TYPE_LONG_INT:
            ui->variablesListWidget->addItem(formatVar(var.first, node, node->Value.v_int64)); break;
        case DATA_TYPE::TYPE_DOUBLE:
            ui->variablesListWidget->addItem(formatVar(var.first, node, node->Value.v_double)); break;
        case DATA_TYPE::TYPE_BOOL:
            ui->variablesListWidget->addItem(formatVar(var.first, node, node->Value.v_bool)); break;
        default: break;
        }
    }
}

void MainWindow::onRobotDataShouldUpdate()
{
    auto& rd = RobotData::getInstance();
    for (int8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        QString const s = QString("[%1 %2 %3]")
            .arg(rd.getX(i))
            .arg(rd.getY(i))
            .arg(rd.getZ(i));
        ui->robotDataListWidget->item(i)->setText(s);
    }
    redrawRobot();
}

void MainWindow::onRecvCurrentStepMsg(QString const& currentStepMsg)
{
    QString s = QString("Шаг #%1: %2")
                    .arg(ui->commandsListWidget->count()+1)
                    .arg(currentStepMsg);
    ui->commandsListWidget->addItem(s);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    cleanupParser();
    QMainWindow::closeEvent(event);
}

void MainWindow::redrawRobot()
{
    if (robotViewWidget == nullptr) return;

    if (ui->actionPlaneXOY->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::Top);
    if (ui->actionPlaneXOZ->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::Front);
    if (ui->actionPlaneYOZ->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::Side);

    robotViewWidget->setInstrumentEnabled(ui->toggleInstrumentAction->isChecked());

    robotViewWidget->update();
}

void MainWindow::onUpdateSegmentLength(uint8_t buttonIndex)
{
    bool ok;
    QString const label(QString("Укажите длину сегмента #%1")
                            .arg(buttonIndex + 1)
    );
    double const len = QInputDialog::getDouble(this,
                                               label,
                                               label,
                                               RobotData::segmentLengths.at(buttonIndex),
                                               RobotData::minSegmentLength,
                                               RobotData::maxSegmentLength,
                                               1,
                                               &ok,
                                               Qt::WindowFlags(),
                                               0.1
    );
    if (!ok) return;
    RobotData::segmentLengths.at(buttonIndex) = len;

    QString txt(QString("Длина сегмента #%1 = %2")
        .arg(buttonIndex + 1)
        .arg(len)
    );
    updateSegmentLengthsActions.at(buttonIndex)->setText(txt);
}

void MainWindow::onUpdatedActionPlane(RobotViewWidget::ViewPlane plane)
{
    ui->actionPlaneXOY->setChecked(plane == RobotViewWidget::ViewPlane::Top);
    ui->actionPlaneXOZ->setChecked(plane == RobotViewWidget::ViewPlane::Front);
    ui->actionPlaneYOZ->setChecked(plane == RobotViewWidget::ViewPlane::Side);

    ui->actionPlaneXOY->setEnabled(plane != RobotViewWidget::ViewPlane::Top);
    ui->actionPlaneXOZ->setEnabled(plane != RobotViewWidget::ViewPlane::Front);
    ui->actionPlaneYOZ->setEnabled(plane != RobotViewWidget::ViewPlane::Side);

    ui->projectionWidgetLabel->setText(QString("Проекция на плоскость %1")
        .arg(plane == RobotViewWidget::ViewPlane::Front ? "XOZ"
        : plane == RobotViewWidget::ViewPlane::Side ? "YOZ" : "XOY")
    );
    redrawRobot();
}

void MainWindow::onLoadedSrcCode()
{
    QString const fileName = QFileDialog::getOpenFileName(
        this,
        tr("Select RML File"),
        "D:\\8 semester\\Diplome\\code",
        tr("All Files (*)")
        );

    if (fileName.isEmpty())
    {
        qDebug() << "User cancelled the dialog.";
        return;
    }

    qDebug() << "User selected:" << fileName;

    // Обработка загрузки файла
    if (!sc->loadFile(fileName.toStdString()))
    {
        qDebug() << "File not loaded into scanner";
        QMessageBox::critical(nullptr, "Ошибка загрузки файла", "Не удалось загрузить исходный код в сканер.");
        return;
    }
    qDebug() << "File loaded into scanner";

    ui->uploadSourceCodeAction->setEnabled(false);
    ui->deleteSourceCodeAction->setEnabled(true);
    ui->updateSegmentAmountAction->setEnabled(false);
    ui->nextStepButton->setEnabled(true);
    ui->workspaceSizeAction->setEnabled(false);
    ui->projectionWidgetLabel->show();
    for (auto it = updateSegmentLengthsActions.begin(); it != updateSegmentLengthsActions.end(); it++)
        (*it)->setEnabled(false);

    robotViewWidget = new RobotViewWidget(679, RobotData::workspaceSize / 2.0, this);
    robotViewWidget->move(ui->projectionWidgetLabel->x(), 61); // на ноутбуке: 71 вместо 61
    robotViewWidget->show();

    parserThrd = new QThread(this);

    dg = new Diagram(sc);
    dg->moveToThread(parserThrd);

    connect(parserThrd, &QThread::started, dg, &Diagram::startParse);
    connect(parserThrd, &QThread::finished, dg, &QObject::deleteLater);
    connect(dg, &Diagram::parseError, this, &MainWindow::onParseError);
    connect(ui->nextStepButton, &QPushButton::clicked, this, [this]() { if (dg) dg->nextStep(); });

    connect(dg, &Diagram::sendAllCurrentVariables, this, &MainWindow::onVariablesReceived);
    connect(dg, &Diagram::sendRobotData, this, &MainWindow::onRobotDataShouldUpdate);
    connect(dg, &Diagram::sendCurrentStepMsg, this, &MainWindow::onRecvCurrentStepMsg);

    auto& rd = RobotData::getInstance();
    rd.reset();
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        QString s = QString("[%1 %2 %3]")
        .arg(rd.getX(i))
            .arg(rd.getY(i))
            .arg(rd.getZ(i));
        ui->robotDataListWidget->addItem(s);
    }

    parserThrd->start();
}

void MainWindow::onParseError(const QString& msg)
{
    qDebug() << "Parser exception:" << msg;
    cleanupParser();
    QMessageBox::critical(nullptr, "Ошибка в процессе интерпретации", msg);
}

void MainWindow::onUpdateWorkspaceSize()
{
    bool ok;
    static QString const label(QString("Укажите размер рабочей области (%1-%2)")
        .arg(RobotData::minWorkspaceSize)
        .arg(RobotData::maxWorkspaceSize)
    );
    double const workspaceSize =
        QInputDialog::getDouble(
            this,
            label,
            label,
            RobotData::workspaceSize,
            RobotData::minWorkspaceSize,
            RobotData::maxWorkspaceSize,
            1,
            &ok,
            Qt::WindowFlags(),
            0.1);
    RobotData::workspaceSize = (ok ? workspaceSize : RobotData::workspaceSize);
    ui->workspaceSizeAction->setText(QString("Размер рабочей области = %1")
        .arg(RobotData::workspaceSize));
}
