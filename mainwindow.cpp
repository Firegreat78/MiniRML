// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "robotdata.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QDebug>
#include <QMessageBox>
#include <QColorDialog>

#include <iostream>

using namespace std;

void MainWindow::setupUi()
{
    ui->setupUi(this);

    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++)
    {
        qsizetype i = it - updateSegmentLengthsActions.begin();

        QPointer<QAction> segmentLenAction(new QAction(this));
        segmentLenAction->setData(i);
        segmentLenAction->setText(tr("Длина сегмента #%1 = %2")
                                      .arg(i + 1)
                                      .arg(RobotData::segmentLengths[i]));
        connect(segmentLenAction, &QAction::triggered, this, &MainWindow::onUpdateSegmentLength);

        ui->menuMain->addAction(segmentLenAction);
        segmentLenAction->setVisible(i < RobotData::segmentAmount);
        *it = std::move(segmentLenAction);
    }

    // enter нажимает кнопку
    enterShortcutNextStepButton = new QShortcut(Qt::Key_Return, this);
    connect(
        enterShortcutNextStepButton,
        &QShortcut::activated,
        ui->nextStepButton,
        &QPushButton::click
        );
    enterShortcutNextStepButton->setEnabled(false);
    ui->nextStepButton->setVisible(false);

    ui->updateSegmentAmountAction->setText(QString("Указать количество сегментов робота (текущее количество = %1)")
        .arg(RobotData::segmentAmount));

    ui->workspaceSizeAction->setText(
        QString("Размер рабочей области = %1")
            .arg(RobotData::workspaceSize)
        );

    connect(ui->workspaceSizeAction, &QAction::triggered, this, &MainWindow::onUpdateWorkspaceSize);

    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++)
    {
        qsizetype i = it - updateSegmentLengthsActions.begin();
        (*it)->setText(tr("Длина сегмента #%1 = %2")
                           .arg(i+1)
                           .arg(RobotData::segmentLengths[i])
                       );
        (*it)->setVisible(i < RobotData::segmentAmount);
    }

    ui->projectionWidgetLabel->hide();

    connect(ui->toggleInstrumentAction, &QAction::triggered, this, &MainWindow::redrawRobot);

    // смена проекции
    connect(ui->actionPlaneXOY, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::XOY); });
    connect(ui->actionPlaneXOZ, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::XOZ); });
    connect(ui->actionPlaneYOZ, &QAction::triggered, this, [this]() { emit updatedActionPlane(RobotViewWidget::ViewPlane::YOZ); });
    connect(this, &MainWindow::updatedActionPlane, this, &MainWindow::onUpdatedActionPlane);

    connect(ui->uploadSourceCodeAction, &QAction::triggered, this, &MainWindow::onLoadedSrcCode);
    connect(ui->deleteSourceCodeAction, &QAction::triggered, this, &MainWindow::cleanupParser);
    ui->deleteSourceCodeAction->setVisible(false);

    connect(ui->updateSegmentAmountAction, &QAction::triggered, this, &MainWindow::onUpdateSegmentAmount);

    ui->deleteColorAction->setEnabled(!RobotViewWidget::colors.empty());

    // цвета по умолчанию
    const QList<QColor> defaultColors = {
        Qt::cyan,            // #00FFFF - Голубой
        Qt::yellow,          // #FFFF00 - Жёлтый
        Qt::magenta,         // #FF00FF - Пурпурный
        QColor(0, 255, 0),   // #00FF00 - Зелёный
        QColor(255, 128, 0), // #FF8000 - Оранжевый
        QColor(255, 0, 128), // #FF0080 - Розовый
        QColor(128, 255, 0), // #80FF00 - Салатовый
        QColor(0, 128, 255)  // #0080FF - Синий
    };

    for (const QColor& color : defaultColors) addColor(color);

    connect(ui->addColorAction, &QAction::triggered, this, &MainWindow::onAddColor);
    connect(ui->deleteColorAction, &QAction::triggered, this, &MainWindow::onDeleteColor);

    connect(ui->tickAmountAction, &QAction::triggered, this, &MainWindow::onChangeTickAmountRequested);
    ui->tickAmountAction->setText(tr("Количество делений = %1").arg(RobotViewWidget::tickAmount));

    connect(ui->tickSizeAction, &QAction::triggered, this, &MainWindow::onChangeTickSizeRequested);
    ui->tickSizeAction->setText(tr("Размер делений = %1").arg(RobotViewWidget::tickSize));

    connect(ui->segmentThicknessAction, &QAction::triggered, this, &MainWindow::onChangeSegmentThicknessRequested);
    ui->segmentThicknessAction->setText(tr("Толщина сегментов = %1").arg(RobotViewWidget::segmentThickness));

    connect(ui->jointSizeAction, &QAction::triggered, this, &MainWindow::onChangeJointSizeRequested);
    ui->jointSizeAction->setText(tr("Размер шарниров = %1").arg(RobotViewWidget::jointSize));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    setupUi();
    sc = new Scanner();
    RobotData::getInstance().reset();
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

    ui->uploadSourceCodeAction->setVisible(true);
    ui->deleteSourceCodeAction->setVisible(false);

    ui->updateSegmentAmountAction->setEnabled(true);

    ui->nextStepButton->setVisible(false);
    enterShortcutNextStepButton->setEnabled(false);
    disconnect(ui->nextStepButton, &QPushButton::clicked, this, &MainWindow::onNextStep);

    ui->workspaceSizeAction->setEnabled(true);

    for (auto it = updateSegmentLengthsActions.begin();
         it != updateSegmentLengthsActions.end();
         it++
    ) (*it)->setEnabled(true);

    ui->variablesListWidget->clear();
    ui->robotDataListWidget->clear();
    ui->commandsListWidget->clear();

    if (dg)
        dg->requestStop();


    if (parserThrd)
    {
        parserThrd->quit();
        parserThrd->wait();
        parserThrd->deleteLater();
    }

    dg = nullptr;

    ui->projectionWidgetLabel->hide();

    if (robotViewWidget)
    {
        robotViewWidget->hide();
        robotViewWidget->deleteLater();
    }

    qDebug() << "Parser deleted";
}

void MainWindow::onUpdateSegmentAmount()
{
    bool ok;
    int const segments = QInputDialog::getInt(
        this,
        "Изменить количество сегментов",
        tr("Укажите количество сегментов (%1-%2):")
            .arg(RobotData::minSegmentAmount)
            .arg(RobotData::maxSegmentAmount),
        RobotData::segmentAmount,
        RobotData::minSegmentAmount,
        RobotData::maxSegmentAmount,
        1,
        &ok);
    if (!ok) return;

    RobotData::segmentAmount = segments;
    QString seg_amt_txt(tr("Указать количество сегментов робота (текущее количество = %1)")
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
        return tr("[%1:%2] %3 %4 = %5")
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
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        QString const s = tr("Шарнир #%1\n[%2 %3 %4]%5")
            .arg(i + 1)
            .arg(rd.getX(i))
            .arg(rd.getY(i))
            .arg(rd.getZ(i))
            .arg(i == robotViewWidget->getHighlightedJoint() ? "\n[выделен]" : "");
        ui->robotDataListWidget->item(i)->setText(s);
    }
    ui->robotDataListWidget->item(RobotData::segmentAmount + 1)->setHidden(
        robotViewWidget->getHighlightedJoint() > RobotData::segmentAmount
    );
    redrawRobot();
}

void MainWindow::onRecvCurrentStepMsg(QString const& currentStepMsg)
{
    QString s = tr("Шаг #%1: %2")
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

    if (ui->actionPlaneXOY->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::XOY);
    if (ui->actionPlaneXOZ->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::XOZ);
    if (ui->actionPlaneYOZ->isChecked()) robotViewWidget->setCurrentPlane(RobotViewWidget::ViewPlane::YOZ);

    robotViewWidget->setInstrumentEnabled(ui->toggleInstrumentAction->isChecked());

    robotViewWidget->update();
}

void MainWindow::onUpdateSegmentLength()
{
    QAction* triggeredAction = qobject_cast<QAction*>(sender());
    if (!triggeredAction) return;

    uint8_t const index = triggeredAction->data().toUInt();

    bool ok;
    QString const label(tr("Укажите длину сегмента #%1")
        .arg(index + 1)
    );

    double const len = QInputDialog::getDouble(
        this,
        label,
        label,
        RobotData::segmentLengths.at(index),
        RobotData::minSegmentLength,
        RobotData::maxSegmentLength,
        1,
        &ok,
        Qt::WindowFlags(),
        0.1
        );
    if (!ok) return;
    RobotData::segmentLengths.at(index) = len;

    QString txt(tr("Длина сегмента #%1 = %2")
                    .arg(index + 1)
                    .arg(len)
                );
    updateSegmentLengthsActions.at(index)->setText(txt);
}

void MainWindow::onUpdatedActionPlane(RobotViewWidget::ViewPlane plane)
{
    ui->actionPlaneXOY->setChecked(plane == RobotViewWidget::ViewPlane::XOY);
    ui->actionPlaneXOZ->setChecked(plane == RobotViewWidget::ViewPlane::XOZ);
    ui->actionPlaneYOZ->setChecked(plane == RobotViewWidget::ViewPlane::YOZ);

    ui->actionPlaneXOY->setEnabled(plane != RobotViewWidget::ViewPlane::XOY);
    ui->actionPlaneXOZ->setEnabled(plane != RobotViewWidget::ViewPlane::XOZ);
    ui->actionPlaneYOZ->setEnabled(plane != RobotViewWidget::ViewPlane::YOZ);

    ui->projectionWidgetLabel->setText(tr("Проекция на плоскость %1")
        .arg(plane == RobotViewWidget::ViewPlane::XOZ ? "XOZ"
        : plane == RobotViewWidget::ViewPlane::YOZ ? "YOZ" : "XOY")
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

    ui->uploadSourceCodeAction->setVisible(false);
    ui->deleteSourceCodeAction->setVisible(true);
    ui->updateSegmentAmountAction->setEnabled(false);

    ui->nextStepButton->setVisible(true);
    enterShortcutNextStepButton->setEnabled(true);

    ui->workspaceSizeAction->setEnabled(false);
    ui->projectionWidgetLabel->show();
    for (auto it = updateSegmentLengthsActions.begin(); it != updateSegmentLengthsActions.end(); it++)
        (*it)->setEnabled(false);

    robotViewWidget = new RobotViewWidget(679, RobotData::workspaceSize / 2.0, this);
    robotViewWidget->move(ui->projectionWidgetLabel->x(), 71); // на ноутбуке: 71 вместо 61
    robotViewWidget->show();

    parserThrd = new QThread(this);

    dg = new Diagram(sc);
    dg->moveToThread(parserThrd);

    connect(parserThrd, &QThread::started, dg, &Diagram::startParse);
    connect(parserThrd, &QThread::finished, dg, &QObject::deleteLater);

    connect(dg, &Diagram::parseFinished, this, &MainWindow::onParseFinished);
    connect(dg, &Diagram::parseError, this, &MainWindow::onParseError);

    connect(ui->nextStepButton, &QPushButton::clicked, this, &MainWindow::onNextStep);

    connect(dg, &Diagram::sendAllCurrentVariables, this, &MainWindow::onVariablesReceived);
    connect(dg, &Diagram::sendRobotData, this, &MainWindow::onRobotDataShouldUpdate);
    connect(dg, &Diagram::sendCurrentStepMsg, this, &MainWindow::onRecvCurrentStepMsg);

    auto& rd = RobotData::getInstance();
    rd.reset();

    connect(ui->robotDataListWidget, &QListWidget::itemClicked, this, &MainWindow::onRobotDataItemClicked);
    for (uint8_t i = 0; i <= RobotData::segmentAmount; i++)
    {
        QListWidgetItem* item = new QListWidgetItem(ui->robotDataListWidget);
        item->setText(tr("Шарнир #%1\n[%2 %3 %4]\n%5")
                          .arg(i + 1)
                          .arg(rd.getX(i))
                          .arg(rd.getY(i))
                          .arg(rd.getZ(i))
                          .arg(RobotViewWidget::getColor(i).name().toUpper()));
        item->setData(Qt::UserRole, i);
    }

    QListWidgetItem* item = new QListWidgetItem(ui->robotDataListWidget);
    item->setText("[снять выделение]");
    item->setData(Qt::UserRole, numeric_limits<decltype(RobotData::segmentAmount)>::max());

    parserThrd->start();
}

void MainWindow::onParseError(const QString& msg)
{
    qDebug() << "Parser exception: " << msg;
    redrawRobot();
    ui->commandsListWidget->addItem(tr("Шаг #%1: %2").arg(ui->commandsListWidget->count() + 1).arg(msg));
    ui->nextStepButton->setVisible(false);
    QMessageBox::critical(nullptr, "Ошибка в процессе интерпретации", msg);
}

void MainWindow::onUpdateWorkspaceSize()
{
    bool ok;
    static QString const label(tr("Укажите размер рабочей области (%1-%2)")
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

void MainWindow::onParseFinished()
{
    ui->nextStepButton->setVisible(false);
    QMessageBox::information(
        this,
        "Parser sucessfully finished working!",
        "Parser had finished finished working. Now you need to cleanup it"
    );
}

void MainWindow::onAddColor()
{
    if (RobotViewWidget::colors.size() >= RobotViewWidget::maxColorAmount)
    {
        QMessageBox::warning(
            this,
            "Задано максимальное количество цветов",
            tr("Было задано максимальное количество цветов (%1)!").arg(RobotViewWidget::maxColorAmount)
        );
        return;
    }
    QColor const color = QColorDialog::getColor(Qt::white, this, tr("Выберите цвет"));

    if (!color.isValid()) return;

    addColor(color);

    onRobotDataShouldUpdate();
}

void MainWindow::onDeleteColor()
{
    if (colorActions.isEmpty())
        return;

    bool ok = true;
    int colorIdx = 0;

    if (colorActions.size() != 1)
    {
        colorIdx = QInputDialog::getInt(
                       this,
                       tr("Удалить цвет"),
                       tr("Укажите номер цвета, который хотите удалить (#1-#%1):")
                           .arg(colorActions.size()),
                       colorActions.size(),
                       1,
                       colorActions.size(),
                       1,
                       &ok) - 1;

        if (!ok) return;
    }
    else
    {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Удалить цвет"),
            tr("Это последний цвет. Удалить его?"),
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply != QMessageBox::Yes) return;
        colorIdx = 0;
    }

    deleteColor(colorIdx);

    onRobotDataShouldUpdate();
}

void MainWindow::addColor(QColor color)
{
    QPointer<QAction> action(new QAction(this));

    connect(action, &QAction::triggered, this, &MainWindow::onEditColor);
    action->setData(static_cast<uint8_t>(colorActions.size()));

    action->setText(tr("Цвет #%1: %2")
        .arg(colorActions.size() + 1)
        .arg(color.name().toUpper())
    );
    ui->menuColors->addAction(action);

    RobotViewWidget::colors.push_back(color);
    colorActions.push_back(std::move(action));

    ui->deleteColorAction->setEnabled(true);
}

void MainWindow::deleteColor(uint8_t validColorIndex)
{
    if (validColorIndex >= colorActions.size())
        return;

    QPointer<QAction> action = colorActions.takeAt(validColorIndex);
    RobotViewWidget::colors.removeAt(validColorIndex);
    ui->menuColors->removeAction(action);
    action->deleteLater();

    for (auto it = colorActions.begin();
         it != colorActions.end();
         it++)
    {
        qsizetype i = it - colorActions.begin();
        (*it)->setText(tr("Цвет #%1: %2")
            .arg(i + 1)
            .arg(RobotViewWidget::colors[i].name().toUpper())
        );
        (*it)->setData(static_cast<uint8_t>(i));
    }

    ui->deleteColorAction->setEnabled(!colorActions.empty());
}

void MainWindow::onEditColor()
{
    QAction* triggeredAction = qobject_cast<QAction*>(sender());
    if (!triggeredAction) return;

    uint8_t const index = triggeredAction->data().toUInt();

    QColor const color = QColorDialog::getColor(
        RobotViewWidget::colors[index],
        this,
        tr("Выберите цвет для шарнира #%1").arg(index + 1)
        );
    if (!color.isValid()) return;

    RobotViewWidget::colors[index] = color;
    triggeredAction->setText(tr("Цвет #%1: %2")
        .arg(index + 1)
        .arg(color.name().toUpper())
    );
    onRobotDataShouldUpdate();
}

void MainWindow::onNextStep()
{
    if (dg) dg->nextStep();
}

void MainWindow::onRobotDataItemClicked(QListWidgetItem* item)
{
    if (!item) return;

    uint8_t jointIndex = static_cast<uint8_t>(item->data(Qt::UserRole).toUInt());
    qDebug() << "Clicked on joint #" << jointIndex + 1;

    if (!robotViewWidget) return;

    robotViewWidget->setHighlightedJoint(jointIndex);
    onRobotDataShouldUpdate();
}

void MainWindow::onChangeTickAmountRequested()
{
    bool ok;
    int const tickAmount = QInputDialog::getInt(
        this,
        tr("Изменить количество делений"),
        tr("Укажите количество делений (%1-%2):")
            .arg(RobotViewWidget::minTickAmount)
            .arg(RobotViewWidget::maxTickAmount),
        RobotViewWidget::tickAmount,
        RobotViewWidget::minTickAmount,
        RobotViewWidget::maxTickAmount,
        1,
        &ok);
    RobotViewWidget::tickAmount = (ok ? tickAmount : RobotViewWidget::tickAmount);
    ui->tickAmountAction->setText(tr("Количество делений = %1").arg(RobotViewWidget::tickAmount));
    if (robotViewWidget) robotViewWidget->update();
}

void MainWindow::onChangeTickSizeRequested()
{
    bool ok;
    int const tickSize = QInputDialog::getInt(
        this,
        tr("Изменить количество делений"),
        tr("Укажите количество делений (%1-%2):")
            .arg(RobotViewWidget::minTickSize)
            .arg(RobotViewWidget::maxTickSize),
        RobotViewWidget::tickSize,
        RobotViewWidget::minTickSize,
        RobotViewWidget::maxTickSize,
        1,
        &ok);
    RobotViewWidget::tickSize = (ok ? tickSize : RobotViewWidget::tickSize);
    ui->tickSizeAction->setText(tr("Размер делений = %1").arg(RobotViewWidget::tickSize));
    if (robotViewWidget) robotViewWidget->update();
}

void MainWindow::onChangeSegmentThicknessRequested()
{
    bool ok;
    int const segmentThickness =
        QInputDialog::getInt(
            this,
            "Изменить толщину сегментов",
            tr("Укажите толщину сегменов (%1-%2)")
                .arg(RobotViewWidget::minSegmentThickness)
                .arg(RobotViewWidget::maxSegmentThickness),
            RobotViewWidget::segmentThickness,
            RobotViewWidget::minSegmentThickness,
            RobotViewWidget::maxSegmentThickness,
            1,
            &ok
        );
    RobotViewWidget::segmentThickness = (ok ? segmentThickness : RobotViewWidget::segmentThickness);
    ui->segmentThicknessAction->setText(tr("Толщина сегментов = %1").arg(RobotViewWidget::segmentThickness));
    if (robotViewWidget) robotViewWidget->update();
}
void MainWindow::onChangeJointSizeRequested()
{
    bool ok;
    int const jointSize =
        QInputDialog::getInt(
            this,
            "Изменить размер шарниров",
            tr("Укажите размер шарниров (%1-%2)")
                .arg(RobotViewWidget::minJointSize)
                .arg(RobotViewWidget::maxJointSize),
            RobotViewWidget::jointSize,
            RobotViewWidget::minJointSize,
            RobotViewWidget::maxJointSize,
            1,
            &ok
        );
    RobotViewWidget::jointSize = (ok ? jointSize : RobotViewWidget::jointSize);
    ui->jointSizeAction->setText(tr("Размер шарниров = %1").arg(RobotViewWidget::jointSize));
    if (robotViewWidget) robotViewWidget->update();
}
