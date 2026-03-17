#pragma once

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <QFile>
#include <QSerialPort>
#include <QTcpSocket>
#include <QUdpSocket>
#include <memory>

#include "CommDebugTool/ProtocolHandler.h"

class RingBuffer {
public:
    explicit RingBuffer(int capacity = 4 * 1024 * 1024);
    void write(const QByteArray &data);
    QByteArray readAll();
    int size() const;
    int capacity() const;
    quint64 takeDroppedBytes();

private:
    QByteArray buffer_;
    int capacity_ = 0;
    int head_ = 0;
    int tail_ = 0;
    int used_ = 0;
    quint64 dropped_ = 0;
    quint64 droppedSinceLastRead_ = 0;
};

class ThreadSafeLogger {
public:
    explicit ThreadSafeLogger(const QString &filePath);
    void log(const QString &direction, const QByteArray &data);
    void setEnabled(bool enabled);
    void setFilePath(const QString &filePath);
    QString filePath() const;

private:
    QMutex mutex_;
    QFile file_;
    QString filePath_;
    bool enabled_ = false;
};

class CommunicationWorker : public QObject {
    Q_OBJECT

public:
    explicit CommunicationWorker(QObject *parent = nullptr);
    ~CommunicationWorker() override;

public slots:
    void startSerial(const QString &portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl);
    void startTcp(const QString &host, quint16 port);
    void startUdp(const QString &host, quint16 port, quint16 localPort, const QString &bindAddress);
    void stop();
    void sendData(const QByteArray &data);
    void setProtocolHandler(std::shared_ptr<BaseProtocol> handler);
    void setLoggingEnabled(bool enabled);
    void setLogFilePath(const QString &filePath);
    void resetDroppedCounter();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &message);
    void dataReceived(const QByteArray &data);
    void droppedBytes(quint64 totalDropped);

private slots:
    void onSerialReadyRead();
    void onTcpReadyRead();
    void onUdpReadyRead();

private:
    enum class Transport {
        None,
        Serial,
        Tcp,
        Udp
    };

    void closeCurrent();
    void drainBuffer();

    Transport transport_ = Transport::None;
    QSerialPort *serial_ = nullptr;
    QTcpSocket *tcp_ = nullptr;
    QUdpSocket *udp_ = nullptr;
    QString udpHost_;
    quint16 udpPort_ = 0;

    RingBuffer ring_;
    QTimer *drainTimer_ = nullptr;
    ThreadSafeLogger logger_;
    std::shared_ptr<BaseProtocol> protocol_;
    quint64 droppedTotal_ = 0;
};
