#include "CommDebugTool/MainWindow.h"
#include "ui_MainWindow.h"

#include <QSerialPortInfo>
#include <QDateTime>
#include <QRegularExpression>
#include <QScrollBar>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QTextDocument>
#include <QTextCursor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui_(new Ui::MainWindow),
      workerThread_(new QThread(this)),
      autoSendTimer_(new QTimer(this)),
      logFlushTimer_(new QTimer(this)) {
    ui_->setupUi(this);

    logFilePath_ = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/comm_debug.log";

    ui_->plainLog->setReadOnly(true);
    ui_->plainLog->setMaximumBlockCount(10000);
    ui_->plainSend->setMaximumHeight(80);

    ui_->comboTransport->addItems({"Serial", "TCP", "UDP"});
    ui_->comboBaud->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    ui_->comboBaud->setCurrentText("115200");

    ui_->comboDataBits->addItem("5", QSerialPort::Data5);
    ui_->comboDataBits->addItem("6", QSerialPort::Data6);
    ui_->comboDataBits->addItem("7", QSerialPort::Data7);
    ui_->comboDataBits->addItem("8", QSerialPort::Data8);
    ui_->comboDataBits->setCurrentIndex(3);

    ui_->comboParity->addItem("None", QSerialPort::NoParity);
    ui_->comboParity->addItem("Even", QSerialPort::EvenParity);
    ui_->comboParity->addItem("Odd", QSerialPort::OddParity);
    ui_->comboParity->addItem("Mark", QSerialPort::MarkParity);
    ui_->comboParity->addItem("Space", QSerialPort::SpaceParity);
    ui_->comboParity->setCurrentIndex(0);

    ui_->comboStopBits->addItem("1", QSerialPort::OneStop);
    ui_->comboStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
    ui_->comboStopBits->addItem("2", QSerialPort::TwoStop);
    ui_->comboStopBits->setCurrentIndex(0);

    ui_->comboFlow->addItem("None", QSerialPort::NoFlowControl);
    ui_->comboFlow->addItem("RTS/CTS", QSerialPort::HardwareControl);
    ui_->comboFlow->addItem("XON/XOFF", QSerialPort::SoftwareControl);
    ui_->comboFlow->setCurrentIndex(0);

    ui_->comboDisplayMode->addItems({"Hex", "ASCII", "Both"});
    ui_->comboDisplayMode->setCurrentIndex(0);
    ui_->checkAutoScroll->setChecked(true);
    ui_->checkFileLog->setChecked(true);
    ui_->labelLogPath->setText(logFilePath_);

    refreshSerialPorts();

    worker_ = new CommunicationWorker();
    worker_->moveToThread(workerThread_);
    connect(workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_->start();

    connect(ui_->btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(ui_->btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
    connect(ui_->btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(ui_->btnClear, &QPushButton::clicked, ui_->plainLog, &QPlainTextEdit::clear);
    connect(ui_->btnSaveLog, &QPushButton::clicked, this, &MainWindow::onSaveLog);
    connect(ui_->btnFindNext, &QPushButton::clicked, this, &MainWindow::onFindNext);
    connect(ui_->lineSearch, &QLineEdit::returnPressed, this, &MainWindow::onFindNext);
    connect(ui_->btnClearStats, &QPushButton::clicked, this, &MainWindow::clearStats);
    connect(ui_->btnChooseLog, &QPushButton::clicked, this, &MainWindow::onChooseLog);
    connect(ui_->btnOpenLog, &QPushButton::clicked, this, &MainWindow::onOpenLog);
    connect(ui_->checkFileLog, &QCheckBox::toggled, this, &MainWindow::onFileLogToggled);
    connect(ui_->checkAutoSend, &QCheckBox::toggled, this, &MainWindow::onAutoSendToggled);
    connect(ui_->comboTransport, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onTransportChanged);
    connect(ui_->btnRefreshPorts, &QPushButton::clicked, this, &MainWindow::refreshSerialPorts);

    connect(worker_, &CommunicationWorker::dataReceived, this, &MainWindow::onDataReceived);
    connect(worker_, &CommunicationWorker::errorOccurred, this, &MainWindow::onWorkerError);
    connect(worker_, &CommunicationWorker::droppedBytes, this, &MainWindow::onDroppedBytes);
    connect(worker_, &CommunicationWorker::connected, this, [this]() {
        ui_->labelStatusValue->setText("Connected");
        appendLog("[INFO] Connected");
    });
    connect(worker_, &CommunicationWorker::disconnected, this, [this]() {
        ui_->labelStatusValue->setText("Disconnected");
        appendLog("[INFO] Disconnected");
    });

    autoSendTimer_->setInterval(ui_->spinAutoInterval->value());
    connect(autoSendTimer_, &QTimer::timeout, this, &MainWindow::onAutoSendTimeout);
    connect(ui_->spinAutoInterval, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) { autoSendTimer_->setInterval(value); });

    logFlushTimer_->setInterval(40);
    connect(logFlushTimer_, &QTimer::timeout, this, &MainWindow::flushLogBuffer);
    logFlushTimer_->start();

    onFileLogToggled(ui_->checkFileLog->isChecked());
    clearStats();
    onTransportChanged(ui_->comboTransport->currentIndex());
}

MainWindow::~MainWindow() {
    flushLogBuffer();
    if (workerThread_->isRunning()) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::BlockingQueuedConnection);
        workerThread_->quit();
        workerThread_->wait();
    }

    delete ui_;
}

void MainWindow::onConnectClicked() {
    const QString transport = ui_->comboTransport->currentText();

    if (transport == "Serial") {
        const QString portName = ui_->comboPort->currentText();
        const int baudRate = ui_->comboBaud->currentText().toInt();
        const int dataBits = ui_->comboDataBits->currentData().toInt();
        const int parity = ui_->comboParity->currentData().toInt();
        const int stopBits = ui_->comboStopBits->currentData().toInt();
        const int flowControl = ui_->comboFlow->currentData().toInt();
        QMetaObject::invokeMethod(worker_, "startSerial", Qt::QueuedConnection,
                                  Q_ARG(QString, portName),
                                  Q_ARG(int, baudRate),
                                  Q_ARG(int, dataBits),
                                  Q_ARG(int, parity),
                                  Q_ARG(int, stopBits),
                                  Q_ARG(int, flowControl));
        return;
    }

    const QString host = ui_->editHost->text().trimmed();
    const quint16 port = static_cast<quint16>(ui_->spinPort->value());

    if (transport == "TCP") {
        QMetaObject::invokeMethod(worker_, "startTcp", Qt::QueuedConnection,
                                  Q_ARG(QString, host), Q_ARG(quint16, port));
        return;
    }

    if (transport == "UDP") {
        const quint16 localPort = static_cast<quint16>(ui_->spinLocalPort->value());
        const QString bindAddress = ui_->editBindAddress->text().trimmed();
        QMetaObject::invokeMethod(worker_, "startUdp", Qt::QueuedConnection,
                                  Q_ARG(QString, host),
                                  Q_ARG(quint16, port),
                                  Q_ARG(quint16, localPort),
                                  Q_ARG(QString, bindAddress));
    }
}

void MainWindow::onDisconnectClicked() {
    QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
}

void MainWindow::onSendClicked() {
    const QString text = ui_->plainSend->toPlainText();
    const bool hexMode = ui_->checkSendHex->isChecked();
    QByteArray payload = encodeInput(text, hexMode);

    if (payload.isEmpty() && !text.isEmpty()) {
        appendLog("[WARN] Invalid send payload");
        return;
    }

    if (ui_->checkAppendCR->isChecked()) {
        payload.append('\r');
    }
    if (ui_->checkAppendLF->isChecked()) {
        payload.append('\n');
    }

    if (payload.isEmpty()) {
        return;
    }

    QMetaObject::invokeMethod(worker_, "sendData", Qt::QueuedConnection,
                              Q_ARG(QByteArray, payload));

    txBytes_ += static_cast<quint64>(payload.size());
    ui_->labelTxValue->setText(formatBytes(txBytes_));

    QString line;
    if (hexMode) {
        line = QString::fromLatin1(payload.toHex(' ')).toUpper();
    } else {
        line = toPrintableAscii(payload);
    }
    enqueueLog(QString("[TX] %1").arg(line));
}

void MainWindow::onAutoSendToggled(bool enabled) {
    if (enabled) {
        autoSendTimer_->start();
    } else {
        autoSendTimer_->stop();
    }
}

void MainWindow::onAutoSendTimeout() {
    onSendClicked();
}

void MainWindow::onDataReceived(const QByteArray &data) {
    if (data.isEmpty()) {
        return;
    }

    rxBytes_ += static_cast<quint64>(data.size());
    ui_->labelRxValue->setText(formatBytes(rxBytes_));

    if (ui_->checkPauseDisplay->isChecked()) {
        return;
    }

    QString line;
    const int mode = ui_->comboDisplayMode->currentIndex();
    if (mode == 0) {
        line = QString::fromLatin1(data.toHex(' ')).toUpper();
    } else if (mode == 1) {
        line = toPrintableAscii(data);
    } else {
        const QString hex = QString::fromLatin1(data.toHex(' ')).toUpper();
        const QString ascii = toPrintableAscii(data);
        line = hex + " | " + ascii;
    }

    enqueueLog(QString("[RX] %1").arg(line));
}

void MainWindow::onWorkerError(const QString &message) {
    appendLog(QString("[ERROR] %1").arg(message));
}

void MainWindow::onTransportChanged(int index) {
    Q_UNUSED(index);

    const QString transport = ui_->comboTransport->currentText();
    const bool serialMode = (transport == "Serial");

    ui_->groupSerial->setVisible(serialMode);
    ui_->groupNetwork->setVisible(!serialMode);
    const bool udpMode = (transport == "UDP");
    ui_->spinLocalPort->setEnabled(udpMode);
    ui_->editBindAddress->setEnabled(udpMode);
    ui_->labelLocalPort->setEnabled(udpMode);
    ui_->labelBind->setEnabled(udpMode);
}

void MainWindow::refreshSerialPorts() {
    ui_->comboPort->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        ui_->comboPort->addItem(info.portName());
    }

    if (ui_->comboPort->count() == 0) {
        ui_->comboPort->addItem("N/A");
    }
}

QByteArray MainWindow::encodeInput(const QString &text, bool hexMode) const {
    if (!hexMode) {
        return text.toUtf8();
    }

    QString cleaned = text;
    cleaned.remove(QRegularExpression("[^0-9A-Fa-f]"));
    if (cleaned.isEmpty()) {
        return {};
    }

    if (cleaned.size() % 2 != 0) {
        cleaned.prepend('0');
    }

    return QByteArray::fromHex(cleaned.toLatin1());
}

QString MainWindow::formatBytes(quint64 bytes) const {
    static const char *suffixes[] = {"B", "KB", "MB", "GB"};
    double value = static_cast<double>(bytes);
    int suffix = 0;
    while (value >= 1024.0 && suffix < 3) {
        value /= 1024.0;
        ++suffix;
    }
    return QString::number(value, 'f', (suffix == 0 ? 0 : 2)) + " " + suffixes[suffix];
}

QString MainWindow::toPrintableAscii(const QByteArray &data) const {
    QString out;
    out.reserve(data.size());
    for (unsigned char byte : data) {
        if (byte == '\r') {
            out.append("\\r");
        } else if (byte == '\n') {
            out.append("\\n");
        } else if (byte == '\t') {
            out.append("\\t");
        } else if (byte >= 32 && byte < 127) {
            out.append(QChar(byte));
        } else {
            out.append('.');
        }
    }
    return out;
}

void MainWindow::enqueueLog(const QString &line) {
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    logBuffer_.append(QString("%1  %2\n").arg(timestamp, line));
}

void MainWindow::flushLogBuffer() {
    if (logBuffer_.isEmpty()) {
        return;
    }
    if (logBuffer_.endsWith('\n')) {
        logBuffer_.chop(1);
    }
    QScrollBar *bar = ui_->plainLog->verticalScrollBar();
    const int previousValue = bar->value();
    const bool autoScroll = ui_->checkAutoScroll->isChecked();
    ui_->plainLog->appendPlainText(logBuffer_);
    if (autoScroll) {
        bar->setValue(bar->maximum());
    } else {
        bar->setValue(previousValue);
    }
    logBuffer_.clear();
}

void MainWindow::clearStats() {
    rxBytes_ = 0;
    txBytes_ = 0;
    droppedBytes_ = 0;
    ui_->labelRxValue->setText(formatBytes(rxBytes_));
    ui_->labelTxValue->setText(formatBytes(txBytes_));
    ui_->labelDroppedValue->setText(formatBytes(droppedBytes_));
    QMetaObject::invokeMethod(worker_, "resetDroppedCounter", Qt::QueuedConnection);
}

void MainWindow::appendLog(const QString &line) {
    const QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    ui_->plainLog->appendPlainText(QString("%1  %2").arg(timestamp, line));
}

void MainWindow::onFindNext() {
    const QString pattern = ui_->lineSearch->text();
    if (pattern.isEmpty()) {
        return;
    }

    QTextDocument::FindFlags flags;
    if (!ui_->plainLog->find(pattern, flags)) {
        QTextCursor cursor = ui_->plainLog->textCursor();
        cursor.movePosition(QTextCursor::Start);
        ui_->plainLog->setTextCursor(cursor);
        ui_->plainLog->find(pattern, flags);
    }
}

void MainWindow::onSaveLog() {
    const QString fileName = QFileDialog::getSaveFileName(this, "Save Log View", QDir::homePath(), "Text Files (*.txt)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog("[ERROR] Unable to save file");
        return;
    }

    file.write(ui_->plainLog->toPlainText().toUtf8());
    file.close();
}

void MainWindow::onChooseLog() {
    const QString fileName = QFileDialog::getSaveFileName(this, "Select Log File", logFilePath_, "Log Files (*.log)");
    if (fileName.isEmpty()) {
        return;
    }

    logFilePath_ = fileName;
    ui_->labelLogPath->setText(logFilePath_);
    QMetaObject::invokeMethod(worker_, "setLogFilePath", Qt::QueuedConnection,
                              Q_ARG(QString, logFilePath_));
}

void MainWindow::onOpenLog() {
    if (logFilePath_.isEmpty()) {
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(logFilePath_));
}

void MainWindow::onFileLogToggled(bool enabled) {
    QMetaObject::invokeMethod(worker_, "setLoggingEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, enabled));
}

void MainWindow::onDroppedBytes(quint64 totalDropped) {
    droppedBytes_ = totalDropped;
    ui_->labelDroppedValue->setText(formatBytes(droppedBytes_));
}
