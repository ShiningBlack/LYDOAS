#ifndef SPECTROMDEBUGPAGE_H
#define SPECTROMDEBUGPAGE_H

#include <QWidget>
#include <QDebug>
#include <QMessageBox>
#include <QIntValidator>

// serial port
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class SpectromDebugPage;
}

class SpectromDebugPage : public QWidget
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit SpectromDebugPage(QSerialPort *serialport = nullptr, QWidget *parent = nullptr);
    ~SpectromDebugPage();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::SpectromDebugPage *ui;
    QSerialPort *serialport  = nullptr;
    QIntValidator *validator = nullptr;

private:
    const QByteArray Frame_Head = QByteArray::fromHex("011A0115");
    const QByteArray Frame_Tail = QByteArray::fromHex("001A0015");
    const QByteArray Frame_0802 = QByteArray::fromHex("0802");      // 采集间隔
    const QByteArray Frame_0205 = QByteArray::fromHex("0205");      // 叠加次数和曝光时间
    const QByteArray Frame_0601 = QByteArray::fromHex("0601");      // 暗电流值
    const QByteArray Frame_0101 = QByteArray::fromHex("0101");      // 触发模式
    const QByteArray Frame_090B = QByteArray::fromHex("090B");      // 光谱仪序列号

    // Reset 复位
    const QByteArray Frame_1000 = QByteArray::fromHex("1000");
    const QByteArray Frame_CRCR = QByteArray::fromHex("45F0");

    const QString InternalTrigger{"内触发"};
    const QString ExternalTrigger{"外触发"};

private:
    bool isSuccess   = false;       // 设置成功标志位
    bool resetFlag   = false;       // 设置复位标志位
    bool settingFlag = false;       // 正在设置标志位

    // 用于区分模块设置时曝光时间还是叠加次数的标志位
    bool flagSetAdd  = false;
    bool flagSetExp  = false;


    quint16 Interval      = 0;          // 采集间隔 0~65535
    quint8  AddTimes      = 0;          // 叠加次数
    quint32 ExposureTime  = 0;          // 曝光时间
    quint8  DarkCurrent   = 0;          // 暗电流值
    QString SpectromNumber;             // 光谱仪序列号

private:
    void clear();

private slots:
    void slot_Reset();

    void slot_SetInterval();
    void slot_SetAddTimes();
    void slot_SetExposureTime();
    void slot_SetDarkCurrentValue();
    void slot_SetTriggerMode();
    void slot_SetSpectromNumber();

    void slot_ReadInterval();
    void slot_ReadAddTimes();
    void slot_ReadExposureTime();
    void slot_ReadDarkCurrentValue();
    void slot_ReadSpectromNumber();
};

#endif // SPECTROMDEBUGPAGE_H
