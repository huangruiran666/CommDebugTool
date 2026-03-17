#pragma once

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <memory>

#include "CommDebugTool/CommunicationWorker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onSendClicked();
    void onAutoSendToggled(bool enabled);
    void onAutoSendTimeout();
    void onDataReceived(const QByteArray &data);
    void onWorkerError(const QString &message);
    void onTransportChanged(int index);
    void refreshSerialPorts();
    void flushLogBuffer();
    void clearStats();
    void onFindNext();
    void onSaveLog();
    void onChooseLog();
    void onOpenLog();
    void onFileLogToggled(bool enabled);
    void onDroppedBytes(quint64 totalDropped);

private:
    QByteArray encodeInput(const QString &text, bool hexMode) const;
    QString formatBytes(quint64 bytes) const;
    QString toPrintableAscii(const QByteArray &data) const;
    void enqueueLog(const QString &line);
    void appendLog(const QString &line);

    Ui::MainWindow *ui_ = nullptr;
    QThread *workerThread_ = nullptr;
    CommunicationWorker *worker_ = nullptr;
    QTimer *autoSendTimer_ = nullptr;
    QTimer *logFlushTimer_ = nullptr;
    QString logBuffer_;
    QString logFilePath_;
    quint64 rxBytes_ = 0;
    quint64 txBytes_ = 0;
    quint64 droppedBytes_ = 0;
};
