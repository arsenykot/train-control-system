#include "mainwindow.h"

#include <QApplication>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Fusion");

    QPalette palette(Qt::white);
    palette.setColor(QPalette::Window, QColor("#f6f3fa"));
    palette.setColor(QPalette::Highlight, QColor("#eaddff"));
    palette.setColor(QPalette::HighlightedText, QColor("#21005d"));
    a.setPalette(palette);

    a.setStyleSheet(QStringLiteral(R"(
        QWidget { font: 15px "Moscow Sans"; color: #292332; }
        QMainWindow, QWidget#centralwidget { background: #f6f3fa; }
        QLabel { font-weight: 600; color: #625b71; }
        QFrame#card { background: white; border: 1px solid #e8e1ee; border-radius: 18px; }
        QLabel#simulationClock {
            background: #eaddff; color: #4f378b;
            border-radius: 16px; padding: 10px 18px; font-size: 17px;
        }
        QPushButton, QComboBox {
            background: #ece6f3; border: 1px solid #ded5e8;
            border-radius: 16px; padding: 9px 16px;
        }
        QPushButton:hover { background: #e1d7ee; }
        QPushButton#startButton { background: #6750a4; color: white; border-color: #6750a4; }
        QPushButton#startButton:hover { background: #57418e; }
        QPushButton:disabled, QPushButton#startButton:disabled {
            background: #eeeaf2; color: #a29aa9; border-color: #eeeaf2;
        }
        QPushButton:focus, QComboBox:focus { border-color: #6750a4; }
        QGraphicsView { background: white; border: 1px solid #e8e1ee; border-radius: 18px; }
        QTableWidget, QListWidget {
            background: white; alternate-background-color: #faf8fc; border: none;
            selection-background-color: #eaddff; selection-color: #21005d;
        }
        QTableWidget::item, QListWidget::item { padding: 18px; }
        QHeaderView::section {
            background: #f3eff8; color: #625b71;
            border: none; padding: 10px 8px; font-weight: 600;
        }
    )"));

    MainWindow w;
    w.show();
    return a.exec();
}
