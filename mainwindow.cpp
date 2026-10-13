#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableWidget>
#include <QTime>
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
    controls->addWidget(new QLabel("Время: 06:00", this));
    controls->addStretch();

    const QStringList buttons = {"Старт", "Пауза", "Шаг +15 мин", "Сброс"};
    for (const QString &text : buttons) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        controls->addWidget(button);
    }

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

    scene->addRect(25, 40, 30, 18, QPen(Qt::blue), QBrush(Qt::blue));
    scene->addText("Поезд №1")->setPos(10, 10);

    layout->addWidget(new QLabel("Расписание", this));
    auto *schedule = new QTableWidget(7, 3, this);
    schedule->setHorizontalHeaderLabels({"Станция", "Прибытие", "Отправление"});
    schedule->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    schedule->verticalHeader()->hide();
    schedule->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(schedule, 1);

    for (int i = 0; i < 7; ++i) {
        QTime arrival = QTime(6, 15).addSecs((i * 13 - 3) * 60);
        QTime departure = QTime(6, 15).addSecs(i * 13 * 60);
        schedule->setItem(i, 0, new QTableWidgetItem("Станция " + QString::number(i + 1)));
        schedule->setItem(i, 1, new QTableWidgetItem(i == 0 ? "—" : arrival.toString("HH:mm")));
        schedule->setItem(i, 2, new QTableWidgetItem(i == 6 ? "—" : departure.toString("HH:mm")));
    }

    layout->addWidget(new QLabel("Журнал событий", this));
    auto *events = new QListWidget(this);
    events->setMaximumHeight(100);
    layout->addWidget(events);
}

MainWindow::~MainWindow()
{
    delete ui;
}
