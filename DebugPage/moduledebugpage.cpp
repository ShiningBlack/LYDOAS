#include "moduledebugpage.h"
#include "qevent.h"
#include "ui_moduledebugpage.h"

extern quint16 CRC16(const QByteArray &frame, const quint16 startIndex, const quint16 endIndex);

ModuleDebugPage::ModuleDebugPage(QSerialPort *serialport, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ModuleDebugPage),
    serialport(serialport)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowTitle("模块调试页");
    setWindowFlags(this->windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint);

    ui->lineEdit_mg_NO->setReadOnly(true);
    ui->lineEdit_mg_H2S->setReadOnly(true);
    ui->lineEdit_mg_NH3->setReadOnly(true);
    ui->lineEdit_mg_NO2->setReadOnly(true);
    ui->lineEdit_mg_SO2->setReadOnly(true);
    ui->lineEdit_ppm_NO->setReadOnly(true);
    ui->lineEdit_ppm_H2S->setReadOnly(true);
    ui->lineEdit_ppm_NH3->setReadOnly(true);
    ui->lineEdit_ppm_NO2->setReadOnly(true);
    ui->lineEdit_ppm_SO2->setReadOnly(true);
    ui->lineEdit_ModuleNumb->setReadOnly(true);
    ui->lineEdit_ModuleState->setReadOnly(true);
    ui->lineEdit_WaveLengthShift->setReadOnly(true);
    ui->lineEdit_SoftwareVersion->setReadOnly(true);
    ui->lineEdit_ChamberPressure->setReadOnly(true);
    ui->lineEdit_ChamberTemperature->setReadOnly(true);
    ui->lineEdit_SpectrometerTempreature->setReadOnly(true);

    // 刷新按钮
    QObject::connect(ui->pushButton_Refresh, &QPushButton::clicked, this, &ModuleDebugPage::refresh);

    // Set infomation
    QObject::connect(ui->pushButton_SetInterval,      &QPushButton::clicked, this, &ModuleDebugPage::slot_SetInterval);
    QObject::connect(ui->pushButton_SetAddTimes,      &QPushButton::clicked, this, &ModuleDebugPage::slot_SetAddTimes);
    QObject::connect(ui->pushButton_SetXenonEnergy,   &QPushButton::clicked, this, &ModuleDebugPage::slot_SetXenonEnergy);
    QObject::connect(ui->pushButton_SetLigthingTimes, &QPushButton::clicked, this, &ModuleDebugPage::slot_SetLigthingTimes);
    QObject::connect(ui->pushButton_ZeroCaliState,    &QPushButton::clicked, this, &ModuleDebugPage::slot_SetZeroCalibration);

    // Read infomation
    QObject::connect(ui->pushButton_ReadInterval,       &QPushButton::clicked, this, [this](){
        ui->lineEdit_Interval->setText(QString::number(Interval));
    });
    QObject::connect(ui->pushButton_ReadAddTimes,       &QPushButton::clicked, this, [this](){
        ui->lineEdit_AddTimes->setText(QString::number(AddTimes));
    });
    QObject::connect(ui->pushButton_ReadXenonEnergy,    &QPushButton::clicked, this, [this](){
        ui->lineEdit_XenonEnergy->setText(QString::number(XenonEnergy));
    });
    QObject::connect(ui->pushButton_ReadLigthingTimes,  &QPushButton::clicked, this, [this](){
        ui->lineEdit_LigthingTimes->setText(QString::number(LigthingTimes));
    });
}

ModuleDebugPage::~ModuleDebugPage()
{
    delete ui;
}

// 重写关闭事件
void ModuleDebugPage::closeEvent(QCloseEvent *event)
{  
    deleteLater();  // 销毁窗口对象：这个函数会自动调用析构函数
    QWidget::closeEvent(event);
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：清空信息栏
 * 函数参数：null
 * 返回对象：null
 * 链接对象：null
 * 修改时间：2023/8/3
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void ModuleDebugPage::clear()
{
    ui->lineEdit_mg_NO->clear();
    ui->lineEdit_mg_H2S->clear();
    ui->lineEdit_mg_NH3->clear();
    ui->lineEdit_mg_NO2->clear();
    ui->lineEdit_mg_SO2->clear();

    ui->lineEdit_ppm_NO->clear();
    ui->lineEdit_ppm_H2S->clear();
    ui->lineEdit_ppm_NH3->clear();
    ui->lineEdit_ppm_SO2->clear();

    ui->lineEdit_Interval->clear();
    ui->lineEdit_AddTimes->clear();
    ui->lineEdit_XenonEnergy->clear();
    ui->lineEdit_LigthingTimes->clear();

    ui->lineEdit_ModuleNumb->clear();
    ui->lineEdit_ModuleState->clear();
    ui->lineEdit_WaveLengthShift->clear();
    ui->lineEdit_SoftwareVersion->clear();
    ui->lineEdit_ChamberPressure->clear();
    ui->lineEdit_ChamberTemperature->clear();
    ui->lineEdit_SpectrometerTempreature->clear();
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/8/1
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void ModuleDebugPage::refresh()
{
    ui->lineEdit_Interval->setText(QString::number(Interval));
    ui->lineEdit_AddTimes->setText(QString::number(AddTimes));
    ui->lineEdit_XenonEnergy->setText(QString::number(XenonEnergy));
    ui->lineEdit_LigthingTimes->setText(QString::number(LigthingTimes));

    ui->lineEdit_ModuleNumb->setText(ModuleNumber);
    ui->lineEdit_SoftwareVersion->setText(SoftwareNumber);
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/8/2
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void ModuleDebugPage::slot_SetZeroCalibration()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }
    commend.append(Frame_Head).append(Frame_0501).append(0x01);

    quint16 crc_V = CRC16(commend, 0, 3);
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    quint8  crc_l = crc_V & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /* debug */
//    qDebug() << Qt::hex << commend;
    qDebug() << commend.toHex(' ');

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        // 发送失败
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入零点校准命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        // 写入部分数据
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入零点校准命令失败！return " << bytesWritten;
    } else {
        // 发送成功
        if (!serialport->waitForBytesWritten(1000)) {
            // 等待超时
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入零点校准命令超时1000ms";
        } else {
            // 等待成功
            settingFlag = true;
            ui->pushButton_ZeroCaliState->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入零点校准命令成功！";
        }
    }
    return;
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/8/1
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void ModuleDebugPage::slot_SetInterval()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    quint8 value = quint8(ui->lineEdit_Interval->text().toInt());
    commend.append(Frame_Head).append(Frame_0101).append(value);

    quint16 crc_V = CRC16(commend, 0, 3);
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    quint8  crc_l = crc_V & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /* debug */
//    qDebug() << Qt::hex << commend;
    qDebug() << commend.toHex(' ');

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        // 发送失败
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        // 写入部分数据
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令失败！return " << bytesWritten;
    } else {
        // 发送成功
        if (!serialport->waitForBytesWritten(1000)) {
            // 等待超时
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令超时1000ms";
        } else {
            // 等待成功
            settingFlag = true;
            ui->pushButton_SetInterval->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令成功！";
        }
    }
    return;
}

void ModuleDebugPage::slot_SetAddTimes()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    quint8 value = quint8(ui->lineEdit_AddTimes->text().toInt());
    commend.append(Frame_Head).append(Frame_0301).append(value);

    quint16 crc_V = CRC16(commend, 0, 3);
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    quint8  crc_l = crc_V & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /* debug */
//    qDebug() << Qt::hex << commend;
    qDebug() << commend.toHex(' ');
//    qDebug() << QDebug::toString(commend.toHex(' '));

//    QDebug::toString(commend.toHex(' '));

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        // 发送失败
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        // 写入部分数据
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令失败！return " << bytesWritten;
    } else {
        // 发送成功
        if (!serialport->waitForBytesWritten(1000)) {
            // 等待超时
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令超时1000ms";
        } else {
            // 等待成功
            settingFlag = true;
            ui->pushButton_SetAddTimes->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令成功！";
        }
    }
    return;
}

void ModuleDebugPage::slot_SetXenonEnergy()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    quint8 value = quint8(ui->lineEdit_XenonEnergy->text().toInt());
    commend.append(Frame_Head).append(Frame_0401).append(value);

    quint16 crc_V = CRC16(commend, 0, 3);
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    quint8  crc_l = crc_V & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /* debug */
    qDebug() << Qt::hex << commend;

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        // 发送失败
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入氙灯能量命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        // 写入部分数据
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入氙灯能量命令失败！return " << bytesWritten;
    } else {
        // 发送成功
        if (!serialport->waitForBytesWritten(1000)) {
            // 等待超时
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入氙灯能量命令超时1000ms";
        } else {
            // 等待成功
            settingFlag = true;
            ui->pushButton_SetXenonEnergy->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入氙灯能量命令成功！";
        }
    }
    return;
}

void ModuleDebugPage::slot_SetLigthingTimes()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    quint8 value = quint8(ui->lineEdit_LigthingTimes->text().toInt());
    commend.append(Frame_Head).append(Frame_0201).append(value);

    quint16 crc_V = CRC16(commend, 0, 3);
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    quint8  crc_l = crc_V & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /* debug */
    qDebug() << Qt::hex << commend;

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        // 发送失败
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入亮灯次数命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        // 写入部分数据
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入亮灯次数命令失败！return " << bytesWritten;
    } else {
        // 发送成功
        if (!serialport->waitForBytesWritten(1000)) {
            // 等待超时
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入亮灯次数命令超时1000ms";
        } else {
            // 等待成功
            settingFlag = true;
            ui->pushButton_SetLigthingTimes->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入亮灯次数命令成功！";
        }
    }
    return;
}

