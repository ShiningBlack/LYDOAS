#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H


#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>

class CircularBuffer {
public:
    CircularBuffer(int maxSize);
    bool write(const QByteArray& data);
    QByteArray read(int maxSize);
    int size() const;

private:
    QByteArray buffer_;
    int maxSize_;
    int writePos_;
    int readPos_;
    QMutex mutex_;
};

#endif // CIRCULARBUFFER_H
