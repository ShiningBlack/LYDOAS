#ifndef MODULEDEBUGPAGE_H
#define MODULEDEBUGPAGE_H

#include <QWidget>
#include <QDebug>
#include <QMessageBox>

// serial port
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
class ModuleDebugPage;
}

class ModuleDebugPage : public QWidget
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit ModuleDebugPage(QSerialPort *serialport = nullptr, QWidget *parent = nullptr);
    ~ModuleDebugPage();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ModuleDebugPage *ui;
    QSerialPort *serialport = nullptr;

private:
    const QByteArray Frame_Head = QByteArray::fromHex("F0");   // 命令帧头
    const QByteArray Frame_Tail = QByteArray::fromHex("FF");   // 命令帧尾
    const QByteArray Frame_0101 = QByteArray::fromHex("0101"); // 采集间隔命令码
    const QByteArray Frame_0201 = QByteArray::fromHex("0201"); // 亮灯次数命令码
    const QByteArray Frame_0301 = QByteArray::fromHex("0301"); // 叠加次数命令码
    const QByteArray Frame_0401 = QByteArray::fromHex("0401"); // 氙灯能量命令码
    const QByteArray Frame_0501 = QByteArray::fromHex("0501"); // 零点校准命令码

private:
    bool isSuccess   = false;       // 设置成功标志位
    bool resetFlag   = false;       // 设置复位标志位
    bool settingFlag = false;       // 正在设置标志位

    int Interval      = 0;          // 采集间隔
    int AddTimes      = 0;          // 叠加次数
    int XenonEnergy   = 0;          // 氙灯能量
    int LigthingTimes = 0;          // 亮灯次数
    int SpactrumType  = 0;          // 光谱类型

    QString ModuleNumber;           // 模块序列号
    QString SoftwareNumber;         // 软件版本号

private:
    void clear();

private slots:
    void refresh();
    void slot_SetInterval();
    void slot_SetAddTimes();
    void slot_SetXenonEnergy();
    void slot_SetLigthingTimes();
    void slot_SetZeroCalibration();
};

#endif // MODULEDEBUGPAGE_H
