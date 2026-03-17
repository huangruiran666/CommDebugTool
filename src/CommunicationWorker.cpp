#include "CommDebugTool/CommunicationWorker.h"

#include <QStandardPaths>
#include <QTextStream>
#include <QHostAddress>
#include <QMutexLocker>

RingBuffer::RingBuffer(int capacity)
    : capacity_(capacity) {
    buffer_.resize(capacity_);
}

void RingBuffer::write(const QByteArray &data) {
    if (data.isEmpty()) {
        return;
    }

    if (capacity_ <= 0) {
        return;
    }

    if (data.size() >= capacity_) {
        const quint64 overflow = static_cast<quint64>(used_ + data.size() - capacity_);
        dropped_ += overflow;
        droppedSinceLastRead_ += overflow;

        const QByteArray tail = data.right(capacity_);
        memcpy(buffer_.data(), tail.constData(), tail.size());
        head_ = 0;
        used_ = tail.size();
        tail_ = used_ % capacity_;
        return;
    }

    const int overflow = used_ + data.size() - capacity_;
    if (overflow > 0) {
        head_ = (head_ + overflow) % capacity_;
        used_ -= overflow;
        dropped_ += overflow;
        droppedSinceLastRead_ += overflow;
    }

    int remaining = data.size();
    int offset = 0;
    while (remaining > 0) {
        int chunk = qMin(remaining, capacity_ - tail_);
        memcpy(buffer_.data() + tail_, data.constData() + offset, chunk);
        tail_ = (tail_ + chunk) % capacity_;
        used_ += chunk;
        offset += chunk;
        remaining -= chunk;
    }
}

QByteArray RingBuffer::readAll() {
    if (used_ == 0) {
        return {};
    }

    QByteArray out;
    out.resize(used_);

    int firstChunk = qMin(used_, capacity_ - head_);
    memcpy(out.data(), buffer_.constData() + head_, firstChunk);

    int remaining = used_ - firstChunk;
    if (remaining > 0) {
        memcpy(out.data() + firstChunk, buffer_.constData(), remaining);
    }

    head_ = (head_ + used_) % capacity_;
    used_ = 0;
    return out;
}

int RingBuffer::size() const {
    return used_;
}

int RingBuffer::capacity() const {
    return capacity_;
}

quint64 RingBuffer::takeDroppedBytes() {
    const quint64 dropped = droppedSinceLastRead_;
    droppedSinceLastRead_ = 0;
    return dropped;
}

ThreadSafeLogger::ThreadSafeLogger(const QString &filePath)
    : file_(filePath),
      filePath_(filePath) {
    enabled_ = file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

void ThreadSafeLogger::log(const QString &direction, const QByteArray &data) {
    if (!enabled_ || !file_.isOpen()) {
        return;
    }

    QMutexLocker locker(&mutex_);
    QTextStream stream(&file_);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    stream << timestamp << " [" << direction << "] "
           << data.size() << " bytes "
           << QString::fromLatin1(data.toHex(' ')).toUpper() << "\n";
    stream.flush();
}

void ThreadSafeLogger::setEnabled(bool enabled) {
    QMutexLocker locker(&mutex_);
    enabled_ = enabled;
    if (!enabled_) {
        if (file_.isOpen()) {
            file_.close();
        }
        return;
    }

    if (!file_.isOpen()) {
        file_.setFileName(filePath_);
        enabled_ = file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
}

void ThreadSafeLogger::setFilePath(const QString &filePath) {
    QMutexLocker locker(&mutex_);
    filePath_ = filePath;
    if (file_.isOpen()) {
        file_.close();
    }
    file_.setFileName(filePath_);
    if (enabled_) {
        enabled_ = file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
}

QString ThreadSafeLogger::filePath() const {
    return filePath_;
}

CommunicationWorker::CommunicationWorker(QObject *parent)
    : QObject(parent),
      ring_(8 * 1024 * 1024),
      logger_(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/comm_debug.log") {
    drainTimer_ = new QTimer(this);
    drainTimer_->setInterval(10);
    connect(drainTimer_, &QTimer::timeout, this, &CommunicationWorker::drainBuffer);
    drainTimer_->start();
}

CommunicationWorker::~CommunicationWorker() {
    stop();
}

void CommunicationWorker::startSerial(const QString &portName, int baudRate, int dataBits, int parity, int stopBits, int flowControl) {
    closeCurrent();

    serial_ = new QSerialPort(this);
    serial_->setPortName(portName);
    serial_->setBaudRate(baudRate);
    serial_->setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
    serial_->setParity(static_cast<QSerialPort::Parity>(parity));
    serial_->setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
    serial_->setFlowControl(static_cast<QSerialPort::FlowControl>(flowControl));

    connect(serial_, &QSerialPort::readyRead, this, &CommunicationWorker::onSerialReadyRead);

    if (!serial_->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Serial open failed: %1").arg(serial_->errorString()));
        serial_->deleteLater();
        serial_ = nullptr;
        return;
    }

    transport_ = Transport::Serial;
    emit connected();
}

void CommunicationWorker::startTcp(const QString &host, quint16 port) {
    closeCurrent();

    tcp_ = new QTcpSocket(this);
    connect(tcp_, &QTcpSocket::readyRead, this, &CommunicationWorker::onTcpReadyRead);
    connect(tcp_, &QTcpSocket::connected, this, &CommunicationWorker::connected);
    connect(tcp_, &QTcpSocket::disconnected, this, &CommunicationWorker::disconnected);
    connect(tcp_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(QString("TCP error: %1").arg(tcp_->errorString()));
    });

    tcp_->connectToHost(host, port);
    transport_ = Transport::Tcp;
}

void CommunicationWorker::startUdp(const QString &host, quint16 port, quint16 localPort, const QString &bindAddress) {
    closeCurrent();

    udp_ = new QUdpSocket(this);
    udpHost_ = host;
    udpPort_ = port;

    QHostAddress bindAddr(bindAddress);
    if (bindAddr.isNull()) {
        bindAddr = QHostAddress::Any;
    }

    if (!udp_->bind(bindAddr, localPort)) {
        emit errorOccurred(QString("UDP bind failed: %1").arg(udp_->errorString()));
        udp_->deleteLater();
        udp_ = nullptr;
        return;
    }

    connect(udp_, &QUdpSocket::readyRead, this, &CommunicationWorker::onUdpReadyRead);

    transport_ = Transport::Udp;
    emit connected();
}

void CommunicationWorker::stop() {
    closeCurrent();
}

void CommunicationWorker::sendData(const QByteArray &data) {
    if (data.isEmpty()) {
        return;
    }

    switch (transport_) {
    case Transport::Serial:
        if (serial_) {
            serial_->write(data);
        }
        break;
    case Transport::Tcp:
        if (tcp_) {
            tcp_->write(data);
        }
        break;
    case Transport::Udp:
        if (udp_) {
            udp_->writeDatagram(data, QHostAddress(udpHost_), udpPort_);
        }
        break;
    case Transport::None:
        break;
    }

    logger_.log("TX", data);
}

void CommunicationWorker::setProtocolHandler(std::shared_ptr<BaseProtocol> handler) {
    protocol_ = std::move(handler);
}

void CommunicationWorker::setLoggingEnabled(bool enabled) {
    logger_.setEnabled(enabled);
}

void CommunicationWorker::setLogFilePath(const QString &filePath) {
    logger_.setFilePath(filePath);
}

void CommunicationWorker::resetDroppedCounter() {
    droppedTotal_ = 0;
    ring_.takeDroppedBytes();
}
void CommunicationWorker::onSerialReadyRead() {
    if (!serial_) {
        return;
    }

    const QByteArray data = serial_->readAll();
    ring_.write(data);
}

void CommunicationWorker::onTcpReadyRead() {
    if (!tcp_) {
        return;
    }

    const QByteArray data = tcp_->readAll();
    ring_.write(data);
}

void CommunicationWorker::onUdpReadyRead() {
    if (!udp_) {
        return;
    }

    while (udp_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(udp_->pendingDatagramSize()));
        udp_->readDatagram(datagram.data(), datagram.size());
        ring_.write(datagram);
    }
}

void CommunicationWorker::closeCurrent() {
    if (serial_) {
        serial_->close();
        serial_->deleteLater();
        serial_ = nullptr;
    }

    if (tcp_) {
        tcp_->disconnectFromHost();
        tcp_->deleteLater();
        tcp_ = nullptr;
    }

    if (udp_) {
        udp_->close();
        udp_->deleteLater();
        udp_ = nullptr;
    }

    if (transport_ != Transport::None) {
        emit disconnected();
    }

    transport_ = Transport::None;
}

void CommunicationWorker::drainBuffer() {
    const QByteArray data = ring_.readAll();
    const quint64 dropped = ring_.takeDroppedBytes();
    if (dropped > 0) {
        droppedTotal_ += dropped;
        emit droppedBytes(droppedTotal_);
    }
    if (data.isEmpty()) {
        return;
    }

    if (protocol_) {
        protocol_->parse(data);
    }

    emit dataReceived(data);
    logger_.log("RX", data);
}
