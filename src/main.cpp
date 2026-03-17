#include <QApplication>
#include <QFontDatabase>

#include "CommDebugTool/MainWindow.h"

static QString buildDarkStyle() {
    return QString::fromLatin1(R"(
        * {
            font-family: "SF Pro Text", "SF Pro Display", "Helvetica Neue", "Noto Sans", Arial;
            font-size: 12px;
        }
        QMainWindow, QWidget {
            background-color: #1E1F22;
            color: #E6E6E6;
        }
        QGroupBox {
            border: 1px solid #343740;
            border-radius: 6px;
            margin-top: 10px;
            padding: 6px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            color: #A8B0C2;
        }
        QPlainTextEdit, QLineEdit, QComboBox, QSpinBox {
            background-color: #15161A;
            border: 1px solid #2C2F36;
            border-radius: 4px;
            padding: 4px;
            selection-background-color: #2B4B7C;
            selection-color: #FFFFFF;
        }
        QPushButton {
            background-color: #2D3B57;
            border: 1px solid #3C4B6E;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #3A4E73;
        }
        QPushButton:pressed {
            background-color: #24324D;
        }
        QCheckBox {
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
        }
        QCheckBox::indicator:unchecked {
            border: 1px solid #3C4B6E;
            background: #1E1F22;
        }
        QCheckBox::indicator:checked {
            border: 1px solid #3C4B6E;
            background: #3A5A8A;
        }
    )");
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(buildDarkStyle());

    MainWindow window;
    window.show();

    return app.exec();
}
