#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QFrame>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
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
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    auto *controls = new QHBoxLayout;
    controls->setSpacing(12);
    layout->addLayout(controls);
    timeLabel = new QLabel(this);
    timeLabel->setObjectName("simulationClock");
    timeLabel->setAlignment(Qt::AlignCenter);
    timeLabel->setMinimumHeight(44);
    controls->addWidget(timeLabel, 1);
    updateTime();

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &MainWindow::stepSimulation);

    const QStringList buttons = {"Старт", "Пауза", "Шаг +15 мин", "Сброс"};
    for (const QString &text : buttons) {
        auto *button = new QPushButton(text, this);
        button->setMinimumHeight(44);
        button->setEnabled(false);
        if (text == "Старт") {
            startButton = button;
            startButton->setObjectName("startButton");
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
        controls->addWidget(button, 1);
    }
    updateControls();

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
    view->setBackgroundBrush(Qt::white);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(view);

    scene->addLine(40, 70, 700, 70, QPen(QColor("#c4b5df"), 6, Qt::SolidLine, Qt::RoundCap));
    for (int i = 0; i < 7; ++i) {
        int x = 40 + i * 110;
        scene->addEllipse(x - 7, 63, 14, 14, QPen(QColor("#6750a4"), 3), QBrush(Qt::white));
        auto *name = scene->addText("Станция " + QString::number(i + 1));
        name->setDefaultTextColor(QColor("#625b71"));
        name->setPos(x - name->boundingRect().width() / 2, 85);
    }

    createTrainItems();
    updateTrainPositions();

    auto *panels = new QHBoxLayout;
    panels->setSpacing(16);
    layout->addLayout(panels, 1);
    auto *scheduleCard = new QFrame(this);
    scheduleCard->setObjectName("card");
    auto *scheduleLayout = new QVBoxLayout(scheduleCard);
    scheduleLayout->setContentsMargins(16, 16, 16, 16);
    panels->addWidget(scheduleCard, 3);
    auto *scheduleHeader = new QHBoxLayout;
    scheduleHeader->addWidget(new QLabel("Расписание", this));
    trainSelector = new QComboBox(this);
    scheduleHeader->addWidget(trainSelector);
    scheduleHeader->addStretch();
    scheduleLayout->addLayout(scheduleHeader);
    connect(trainSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::selectTrain);

    schedule = new QTableWidget(0, 3, this);
    schedule->setHorizontalHeaderLabels({"Станция", "Прибытие", "Отправление"});
    schedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    schedule->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    schedule->verticalHeader()->hide();
    schedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    schedule->setShowGrid(false);
    schedule->setAlternatingRowColors(true);
    schedule->setSelectionBehavior(QAbstractItemView::SelectRows);
    schedule->setMinimumHeight(200);
    scheduleLayout->addWidget(schedule, 1);

    updateTrainSelector();
    updateSchedule();

    auto *eventsCard = new QFrame(this);
    eventsCard->setObjectName("card");
    auto *eventsLayout = new QVBoxLayout(eventsCard);
    eventsLayout->setContentsMargins(16, 16, 16, 16);
    panels->addWidget(eventsCard, 2);
    eventsLayout->addWidget(new QLabel("Журнал событий", this));
    events = new QListWidget(this);
    events->setWordWrap(true);
    eventsLayout->addWidget(events, 1);
    updateEvents();

    auto *content = takeCentralWidget();
    auto *workspace = new QScrollArea(this);
    workspace->setWidgetResizable(true);
    workspace->setFrameShape(QFrame::NoFrame);
    workspace->setWidget(content);
    setCentralWidget(workspace);
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
    updateTrainPositions();
    updateEvents();
    if (simulation.finished()) timer->stop();
    updateControls();
}

void MainWindow::resetSimulation()
{
    timer->stop();
    simulation.reset();
    updateTrainSelector();
    events->clear();
    shownEventCount = 0;
    updateTime();
    updateSchedule();
    createTrainItems();
    updateTrainPositions();
    updateEvents();
    updateControls();
}

void MainWindow::updateTime()
{
    QTime time = QTime(0, 0).addSecs(static_cast<int>(simulation.current_time() * 60));
    timeLabel->setText(time.toString("HH:mm"));
}

void MainWindow::selectTrain(int index)
{
    selectedTrain = index;
    updateTrainPositions();
    updateSchedule();
}

void MainWindow::updateTrainSelector()
{
    trainSelector->blockSignals(true);
    trainSelector->clear();
    for (int i = 0; i < simulation.train_count(); ++i) {
        trainSelector->addItem("Поезд №" + QString::number(simulation.train(i).id() + 1));
    }

    if (simulation.train_count() == 0) {
        selectedTrain = -1;
    } else if (selectedTrain < 0 || selectedTrain >= simulation.train_count()) {
        selectedTrain = 0;
    }
    trainSelector->setCurrentIndex(selectedTrain);
    trainSelector->setEnabled(simulation.train_count() > 0);
    trainSelector->blockSignals(false);
}

void MainWindow::updateSchedule()
{
    schedule->setRowCount(0);
    if (selectedTrain < 0 || selectedTrain >= simulation.train_count()) return;

    const Route &route = simulation.route(selectedTrain);
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

void MainWindow::createTrainItems()
{
    for (auto *item : trainItems) {
        delete item;
    }
    trainItems.clear();

    for (int i = 0; i < simulation.train_count(); ++i) {
        auto *item = scene->addRect(22, 38, 36, 22, QPen(Qt::NoPen), QBrush(QColor("#6750a4")));
        auto *number = new QGraphicsSimpleTextItem(QString::number(simulation.train(i).id() + 1), item);
        number->setBrush(Qt::white);
        number->setPos(40 - number->boundingRect().width() / 2,
                       49 - number->boundingRect().height() / 2);
        trainItems.append(item);
    }

}

void MainWindow::updateTrainPositions()
{
    int maxLane = 0;
    for (int i = 0; i < trainItems.size(); ++i) {
        const Train &train = simulation.train(i);
        const Route &route = simulation.route(i);
        int stationId = route.stop(train.cur_state()).s_id;
        double offsetX = (stationId + train.pos_km() / simulation.segment_length_km) * 110;
        int lane = 0;
        for (int j = 0; j < i; ++j) {
            if (qAbs(trainItems[j]->pos().x() - offsetX) < 40)
                lane = qMax(lane, qRound(-trainItems[j]->pos().y() / 24) + 1);
        }
        maxLane = qMax(maxLane, lane);
        trainItems[i]->setPos(offsetX, -lane * 24);
        trainItems[i]->setBrush(QColor(i == selectedTrain ? "#6750a4" : "#7a6a91"));
    }
    scene->setSceneRect(0, 28 - maxLane * 24, 740, 100 + maxLane * 24);
    view->setMinimumHeight(126 + maxLane * 24);
}

void MainWindow::updateEvents()
{
    const auto &log = simulation.log();
    while (shownEventCount < log.size()) {
        events->addItem(log[shownEventCount].text);
        ++shownEventCount;
    }
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
