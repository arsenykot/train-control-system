#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Контроль движения электропоездов");
    setMinimumSize(800, 600);

    auto *layout = new QVBoxLayout(ui->centralwidget);
    auto *controls = new QHBoxLayout;
    layout->addLayout(controls);
    timeLabel = new QLabel(this);
    controls->addWidget(timeLabel);
    updateTime();
    controls->addStretch();

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &MainWindow::stepSimulation);

    const QStringList buttons = {"Старт", "Пауза", "Шаг +15 мин", "Сброс"};
    for (const QString &text : buttons) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        if (text == "Старт") {
            startButton = button;
            connect(button, &QPushButton::clicked, this, &MainWindow::startSimulation);
        } else if (text == "Пауза") {
            pauseButton = button;
            connect(button, &QPushButton::clicked, this, &MainWindow::pauseSimulation);
        } else if (text == "Шаг +15 мин") {
            stepButton = button;
            connect(button, &QPushButton::clicked, this, &MainWindow::stepSimulation);
        } else if (text == "Сброс") {
            button->setEnabled(true);
            connect(button, &QPushButton::clicked, this, &MainWindow::resetSimulation);
        }
        controls->addWidget(button);
    }
    updateControls();

    layout->addWidget(new QLabel("Схема участка", this));
    auto *scene = new QGraphicsScene(this);
    auto *view = new QGraphicsView(scene, this);
    scene->setSceneRect(0, 0, 740, 130);
    view->setMinimumHeight(170);
    layout->addWidget(view);

    scene->addLine(40, 70, 700, 70, QPen(Qt::darkGray, 3));
    for (int i = 0; i < 7; ++i) {
        int x = 40 + i * 110;
        scene->addEllipse(x - 5, 65, 10, 10, QPen(Qt::black), QBrush(Qt::white));
        auto *name = scene->addText("Станция " + QString::number(i + 1));
        name->setPos(x - name->boundingRect().width() / 2, 85);
    }

    trainItem = scene->addRect(25, 40, 30, 18, QPen(Qt::blue), QBrush(Qt::blue));
    updateTrainPosition();
    scene->addText("Поезд №1")->setPos(10, 10);

    layout->addWidget(new QLabel("Расписание", this));
    schedule = new QTableWidget(0, 3, this);
    schedule->setHorizontalHeaderLabels({"Станция", "Прибытие", "Отправление"});
    schedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    schedule->verticalHeader()->hide();
    schedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(schedule, 1);

    updateSchedule();

    layout->addWidget(new QLabel("Журнал событий", this));
    auto *events = new QListWidget(this);
    events->setMaximumHeight(100);
    layout->addWidget(events);
}

void MainWindow::startSimulation()
{
    if (simulation.finished()) return;
    timer->start();
    updateControls();
}

void MainWindow::pauseSimulation()
{
    timer->stop();
    updateControls();
}

void MainWindow::stepSimulation()
{
    simulation.step();
    updateTime();
    updateTrainPosition();
    if (simulation.finished()) timer->stop();
    updateControls();
}

void MainWindow::resetSimulation()
{
    timer->stop();
    simulation.reset();
    updateTime();
    updateSchedule();
    updateTrainPosition();
    updateControls();
}

void MainWindow::updateTime()
{
    QTime time = QTime(0, 0).addSecs(static_cast<int>(simulation.current_time() * 60));
    timeLabel->setText("Время: " + time.toString("HH:mm"));
}

void MainWindow::updateSchedule()
{
    schedule->setRowCount(0);
    if (simulation.train_count() == 0) return;

    const Route &route = simulation.route(0);
    schedule->setRowCount(route.stop_cnt());

    for (int i = 0; i < route.stop_cnt(); ++i) {
        int stationId = route.stop(i).s_id;
        QTime arrival = QTime(0, 0).addSecs(static_cast<int>(route.p_arr(i) * 60));
        QTime departure = QTime(0, 0).addSecs(static_cast<int>(route.p_dep(i) * 60));
        QString arrivalText = "—";
        QString departureText = "—";
        if (i > 0) arrivalText = arrival.toString("HH:mm");
        if (i < route.stop_cnt() - 1) departureText = departure.toString("HH:mm");

        schedule->setItem(i, 0, new QTableWidgetItem(simulation.station(stationId).name()));
        schedule->setItem(i, 1, new QTableWidgetItem(arrivalText));
        schedule->setItem(i, 2, new QTableWidgetItem(departureText));
    }
}

void MainWindow::updateTrainPosition()
{
    if (simulation.train_count() == 0) {
        trainItem->hide();
        return;
    }
    trainItem->show();

    const Train &train = simulation.train(0);
    const Route &route = simulation.route(0);
    int stationId = route.stop(train.cur_state()).s_id;
    double offsetX = (stationId + train.pos_km() / simulation.segment_length_km) * 110;
    trainItem->setPos(offsetX, 0);
}

void MainWindow::updateControls()
{
    if (timer->isActive()) {
        startButton->setEnabled(false);
        pauseButton->setEnabled(true);
        stepButton->setEnabled(false);
    } else if (simulation.finished()) {
        startButton->setEnabled(false);
        pauseButton->setEnabled(false);
        stepButton->setEnabled(false);
    } else {
        startButton->setEnabled(true);
        pauseButton->setEnabled(false);
        stepButton->setEnabled(true);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
