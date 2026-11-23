#include "mainwindow.h"

#include <QApplication>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QPalette palette(Qt::white);
    palette.setColor(QPalette::Window, QColor("#f3f6fb"));
    palette.setColor(QPalette::Highlight, QColor("#2563eb"));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    a.setPalette(palette);

    a.setStyleSheet(QStringLiteral(R"(
        QWidget { font: 13px "Moscow sans"; color: #1e293b; }
        QMainWindow, QWidget#centralwidget { background: #f3f6fb; }
        QLabel { font-weight: 600; }
        QLabel#simulationClock {
            background: #e8efff; color: #1d4ed8;
            border-radius: 10px; padding: 10px 16px; font-size: 17px;
        }
        QPushButton, QComboBox {
            background: white; border: 1px solid #d5deeb;
            border-radius: 8px; padding: 8px 14px;
        }
        QPushButton:hover { background: #edf3ff; }
        QPushButton#startButton { background: #2563eb; color: white; }
        QPushButton#startButton:hover { background: #1d4ed8; }
        QPushButton:disabled, QPushButton#startButton:disabled {
            background: #e9eef5; color: #94a3b8;
        }
        QPushButton:focus, QComboBox:focus { border-color: #2563eb; }
        QGraphicsView, QTableWidget, QListWidget {
            background: white; border: 1px solid #dce4ef; border-radius: 10px;
            selection-background-color: #e8efff; selection-color: #1d4ed8;
        }
        QTableWidget { gridline-color: #edf1f7; }
        QTableWidget::item, QListWidget::item { padding: 4px 10px; }
        QHeaderView::section {
            background: #edf2f9; color: #64748b;
            border: none; padding: 8px; font-weight: 600;
        }
        QComboBox { min-width: 110px; }
    )"));

    MainWindow w;
    w.show();
    return a.exec();
}
