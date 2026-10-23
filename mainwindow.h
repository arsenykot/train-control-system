#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "simulation.h"

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;
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

    Simulation simulation;

    QLabel *timeLabel;
    QTableWidget *schedule;
    QGraphicsRectItem *trainItem;
    QTimer *timer;
    QPushButton *startButton;
    QPushButton *pauseButton;
    QPushButton *stepButton;
    void startSimulation();
    void pauseSimulation();
    void stepSimulation();
    void resetSimulation();
    void updateTime();
    void updateSchedule();
    void updateTrainPosition();
    void updateControls();
};
#endif // MAINWINDOW_H
