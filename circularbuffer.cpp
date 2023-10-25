#include "circularbuffer.h"

CircularBuffer::CircularBuffer(int maxSize)
    : buffer_(maxSize, 0), maxSize_(maxSize), writePos_(0), readPos_(0){
}

bool CircularBuffer::write(const QByteArray& data) {
    QMutexLocker locker(&mutex_);

    int bytesToWrite = data.size();
    if (bytesToWrite + size() > maxSize_) {
        return false;  // 没有足够的空间写入数据
    }

    int firstChunkSize = qMin(bytesToWrite, maxSize_ - writePos_);
    int secondChunkSize = bytesToWrite - firstChunkSize;

    // Write the first chunk
    buffer_.replace(writePos_, firstChunkSize, data.constData(), firstChunkSize);
    writePos_ = (writePos_ + firstChunkSize) % maxSize_;

    // Write the remaining data if any
    if (secondChunkSize > 0) {
        buffer_.replace(writePos_, secondChunkSize, data.constData() + firstChunkSize, secondChunkSize);
        writePos_ = (writePos_ + secondChunkSize) % maxSize_;
    }

    return true;
}

QByteArray CircularBuffer::read(int maxSize) {
    QMutexLocker locker(&mutex_);

    int bytesToRead = qMin(maxSize, size());
    QByteArray data(bytesToRead, 0);

    if (bytesToRead > 0) {
        int firstChunkSize = qMin(bytesToRead, maxSize_ - readPos_);
        int secondChunkSize = bytesToRead - firstChunkSize;

        // Read the first chunk
        data.replace(0, firstChunkSize, buffer_.constData() + readPos_, firstChunkSize);
        readPos_ = (readPos_ + firstChunkSize) % maxSize_;

        // Read the remaining data if any
        if (secondChunkSize > 0) {
            data.replace(firstChunkSize, secondChunkSize, buffer_.constData() + readPos_, secondChunkSize);
            readPos_ = (readPos_ + secondChunkSize) % maxSize_;
        }
    }

    return data;
}

int CircularBuffer::size() const {
    return (writePos_ - readPos_ + maxSize_) % maxSize_;
}
