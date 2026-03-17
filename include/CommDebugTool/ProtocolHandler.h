#pragma once

#include <QByteArray>

class BaseProtocol {
public:
    virtual ~BaseProtocol() = default;
    virtual void parse(const QByteArray &data) = 0;
};
