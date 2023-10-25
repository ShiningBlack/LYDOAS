#include "spectromdebugpage.h"
#include "qevent.h"
#include "ui_spectromdebugpage.h"

extern quint16 CRC16(const QByteArray &frame, const quint16 startIndex, const quint16 endIndex);

SpectromDebugPage::SpectromDebugPage(QSerialPort *serialport, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SpectromDebugPage),
    serialport(serialport)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowTitle("光谱调试");
    setWindowFlags(this->windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint);

    ui->comboBox_TriggerMode->addItem(InternalTrigger);
    ui->comboBox_TriggerMode->addItem(ExternalTrigger);

    // 限制输入的数据格式
    validator = new QIntValidator(0, 99999, this);
    ui->lineEdit_Interval->setValidator(validator);
    ui->lineEdit_AddTimes->setValidator(validator);
    ui->lineEdit_ExposureTime->setValidator(validator);
    ui->lineEdit_DarkCurrentValue->setValidator(validator);

    // 禁用采集间隔
    ui->lineEdit_Interval->setDisabled(true);
    ui->pushButton_SetInterval->setDisabled(true);
    ui->pushButton_ReadInterval->setDisabled(true);

    // 根据正则表达式指定输入格式
    QRegularExpressionValidator *validator1 = new QRegularExpressionValidator(QRegularExpression("[a-zA-Z0-9]+"), ui->lineEdit_SerialNumber);
    ui->lineEdit_SerialNumber->setValidator(validator1);

    ui->lineEdit_SpectromTemp->setReadOnly(true);
    ui->lineEdit_SpectromNumber->setReadOnly(true);
    ui->lineEdit_SoftwareVersion->setReadOnly(true);

    QObject::connect(ui->pushButton_Reset, &QPushButton::clicked, this, &SpectromDebugPage::slot_Reset);

    QObject::connect(ui->pushButton_SetInterval,          &QPushButton::clicked, this, &SpectromDebugPage::slot_SetInterval);
    QObject::connect(ui->pushButton_SetAddTimes,          &QPushButton::clicked, this, &SpectromDebugPage::slot_SetAddTimes);
    QObject::connect(ui->pushButton_SetExposureTime,      &QPushButton::clicked, this, &SpectromDebugPage::slot_SetExposureTime);
    QObject::connect(ui->pushButton_SetDarkCurrentValue,  &QPushButton::clicked, this, &SpectromDebugPage::slot_SetDarkCurrentValue);
    QObject::connect(ui->pushButton_SetTriggerMode,       &QPushButton::clicked, this, &SpectromDebugPage::slot_SetTriggerMode);
    QObject::connect(ui->pushButton_SetSerialNumber,      &QPushButton::clicked, this, &SpectromDebugPage::slot_SetSpectromNumber);

    QObject::connect(ui->pushButton_ReadInterval,         &QPushButton::clicked, this, &SpectromDebugPage::slot_ReadInterval);
    QObject::connect(ui->pushButton_ReadAddTimes,         &QPushButton::clicked, this, &SpectromDebugPage::slot_ReadAddTimes);
    QObject::connect(ui->pushButton_ReadExposureTime,     &QPushButton::clicked, this, &SpectromDebugPage::slot_ReadExposureTime);
    QObject::connect(ui->pushButton_ReadDarkCurrentValue, &QPushButton::clicked, this, &SpectromDebugPage::slot_ReadDarkCurrentValue);
    QObject::connect(ui->pushButton_ReadSerialNumber,     &QPushButton::clicked, this, &SpectromDebugPage::slot_ReadSpectromNumber);
}

SpectromDebugPage::~SpectromDebugPage()
{
    delete ui;
}

void SpectromDebugPage::closeEvent(QCloseEvent *event)
{
    deleteLater();
    QWidget::closeEvent(event);
}

void SpectromDebugPage::clear()
{
    ui->lineEdit_Interval->clear();
    ui->lineEdit_AddTimes->clear();
    ui->lineEdit_ExposureTime->clear();
    ui->lineEdit_DarkCurrentValue->clear();
    ui->lineEdit_SerialNumber->clear();
    ui->lineEdit_SpectromNumber->clear();
    ui->lineEdit_SoftwareVersion->clear();
    ui->lineEdit_SpectromTemp->clear();
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：恢复出厂设置
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/8/1
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void SpectromDebugPage::slot_Reset()
{
    QByteArray commend;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    commend.append(Frame_Head).append(Frame_1000).append(Frame_CRCR).append(Frame_Tail);
    qDebug() << commend.toHex(' '); // debug

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入恢复出厂设置命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入恢复出厂设置命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入恢复出厂设置命令超时1000ms";
        } else {
            settingFlag = true;
            ui->pushButton_Reset->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入恢复出厂设置命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_SetInterval()
{
    QByteArray commend;
    quint16    value;
    quint8     value_h;
    quint8     value_l;
    quint16    crc16;
    quint8     crc16_h;
    quint8     crc16_l;

    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    value = quint16(ui->lineEdit_Interval->text().toInt());
    value_h = static_cast<quint8>((value >> 8) & 0xFF);
    value_l = static_cast<quint8>((value)      & 0xFF);
    commend.append(Frame_Head).append(Frame_0802).append(value_l).append(value_h);

    crc16   = CRC16(commend, 0, commend.size() - 1);
    crc16_l = crc16 & 0xFF;
    crc16_h = (crc16 >> 8) & 0xFF;
    commend.append(crc16_h).append(crc16_l).append(Frame_Tail);

    /********************** 调试 *************************/
    QString stringHexCmd = commend.toHex(' ');
    qDebug() << stringHexCmd;
    /****************************************************/

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令超时1000ms";
        } else {
            settingFlag = true;
            ui->pushButton_SetInterval->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入采集间隔命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_SetAddTimes()
{
    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    QByteArray commend;
    quint8     value;
    quint16    crc16;
    quint8     crc16_h;
    quint8     crc16_l;

    const quint8 byte1 = static_cast<quint8>((ExposureTime)       & 0xFF);
    const quint8 byte2 = static_cast<quint8>((ExposureTime >> 8)  & 0xFF);
    const quint8 byte3 = static_cast<quint8>((ExposureTime >> 16) & 0xFF);
    const quint8 byte4 = static_cast<quint8>((ExposureTime >> 24) & 0xFF);

    value = quint8(ui->lineEdit_AddTimes->text().toInt());
    if (value > 10) {
        QMessageBox::information(this, "tip", "叠加次数需小于等于10！");
        return;
    }
    commend.append(Frame_Head).append(Frame_0205).append(byte1).append(byte2).append(byte3).append(byte4).append(value);

    crc16   = CRC16(commend, 0, commend.size() - 1);
    crc16_l = crc16 & 0xFF;
    crc16_h = (crc16 >> 8) & 0xFF;
    commend.append(crc16_h).append(crc16_l).append(Frame_Tail);

    /********************** 调试 *************************/
    QString stringHexCmd = commend.toHex(' ');
    qDebug() << stringHexCmd;
    /****************************************************/

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令超时1000ms";
        } else {
            flagSetAdd  = true;
            settingFlag = true;
            ui->pushButton_SetAddTimes->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入叠加次数命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_SetExposureTime()
{
    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    QByteArray commend;
    quint16    crc16;
    quint8     crc16_h;
    quint8     crc16_l;

    const quint32 value = ui->lineEdit_ExposureTime->text().toUInt();
    const quint8  a = static_cast<quint8>(value & 0xFF);
    const quint8  b = static_cast<quint8>((value >> 8)   & 0xFF);
    const quint8  c = static_cast<quint8>((value >> 16)  & 0xFF);
    const quint8  d = static_cast<quint8>((value >> 24)  & 0xFF);
    const quint8  e = static_cast<quint8>(AddTimes & 0xFF);
    commend.append(Frame_Head).append(Frame_0205).append(a).append(b).append(c).append(d).append(e);

    crc16   = CRC16(commend, 0, commend.size() - 1);
    crc16_l = crc16 & 0xFF;
    crc16_h = (crc16 >> 8) & 0xFF;
    commend.append(crc16_h).append(crc16_l).append(Frame_Tail);

    /********************** 调试 *************************/
    QString stringHexCmd = commend.toHex(' ');
    qDebug() << stringHexCmd;
    /****************************************************/

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入曝光时间命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入曝光时间命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入曝光时间命令超时1000ms";
        } else {
            flagSetExp  = true;
            settingFlag = true;
            ui->pushButton_SetExposureTime->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入曝光时间命令成功！";
        }
    }
    return;
}


void SpectromDebugPage::slot_SetDarkCurrentValue()
{
    if (!serialport->isOpen()) {
        QMessageBox::warning(this, "LYDOAS", "没有可用串口！");
        return;
    }

    const quint32 literal = ui->lineEdit_DarkCurrentValue->text().toUInt();
    if (literal > 255) {
        QMessageBox::information(this, "LYDOAS", "请输入正确的暗电流值取值范围：0~255");
        return;
    }

    QByteArray commend;
    const quint8 value = static_cast<quint8>(literal & 0xFF);
    commend.append(Frame_Head).append(Frame_0601).append(value);

    quint16 crc_V = CRC16(commend, 0, commend.size() - 1);
    quint8  crc_l = crc_V & 0xFF;
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /********************** 调试 *************************/
    QString stringHexCmd = commend.toHex(' ');
    qDebug() << stringHexCmd;
    /****************************************************/

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入暗电流值命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入暗电流值命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入暗电流值命令超时1000ms";
        } else {
            settingFlag = true;
            ui->pushButton_SetDarkCurrentValue->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入暗电流值命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_SetTriggerMode()
{
    QByteArray commend;
    const quint8 internalTrigger = 0x00;
    const quint8 externalTrigger = 0x01;

    if (ui->comboBox_TriggerMode->currentText() == InternalTrigger) {
        commend.append(Frame_Head).append(Frame_0101).append(internalTrigger);

        quint16 crc_V = CRC16(commend, 0, commend.size() - 1);
        quint8  crc_l = crc_V & 0xFF;
        quint8  crc_h = (crc_V >> 8) & 0xFF;

        commend.append(crc_h).append(crc_l).append(Frame_Tail);

    }else if (ui->comboBox_TriggerMode->currentText() == ExternalTrigger) {
        commend.append(Frame_Head).append(Frame_0101).append(externalTrigger);

        quint16 crc_V = CRC16(commend, 0, commend.size() - 1);
        quint8  crc_l = crc_V & 0xFF;
        quint8  crc_h = (crc_V >> 8) & 0xFF;

        commend.append(crc_h).append(crc_l).append(Frame_Tail);
    }

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令超时1000ms";
        } else {
            settingFlag = true;
//            ui->pushButton_SetTriggerMode->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_SetSpectromNumber()
{
    QByteArray commend;
    commend.append(Frame_Head).append(Frame_090B);

    const QString SerialNumber_QString = ui->lineEdit_SerialNumber->text();
    if (SerialNumber_QString.length() != 11) {
        QMessageBox::information(this, "LYDOAS", "请输入11位光谱仪序列号！");
        return;
    }
    const QByteArray Frame_SerialNumber = SerialNumber_QString.toUtf8();
    qDebug() << "Frame_SerialNumber: " << Frame_SerialNumber;
    commend.append(Frame_SerialNumber);

    quint16 crc_V = CRC16(commend, 0, commend.size() - 1);
    quint8  crc_l = crc_V & 0xFF;
    quint8  crc_h = (crc_V >> 8) & 0xFF;
    commend.append(crc_h).append(crc_l).append(Frame_Tail);

    /********************** 调试 *************************/
    QString stringHexCmd = commend.toHex(' ');
    qDebug() << stringHexCmd;
    /****************************************************/

    qint64 bytesWritten = serialport->write(commend);
    if (bytesWritten == -1) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令失败！return -1";
    } else if (bytesWritten != commend.size()) {
        qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令失败！return " << bytesWritten;
    } else {
        if (!serialport->waitForBytesWritten(1000)) {
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令超时1000ms";
        } else {
            settingFlag = true;
            ui->pushButton_SetSerialNumber->setDisabled(true);
            qDebug() << __LINE__ << __FUNCTION__ << "模块写入触发模式命令成功！";
        }
    }
    return;
}

void SpectromDebugPage::slot_ReadInterval()
{
    ui->lineEdit_Interval->setText(QString::number(Interval));
    return;
}

void SpectromDebugPage::slot_ReadAddTimes()
{
    ui->lineEdit_AddTimes->setText(QString::number(AddTimes));
    return;
}

void SpectromDebugPage::slot_ReadExposureTime()
{
    ui->lineEdit_ExposureTime->setText(QString::number(ExposureTime));
    return;
}

void SpectromDebugPage::slot_ReadDarkCurrentValue()
{
    ui->lineEdit_DarkCurrentValue->setText(QString::number(DarkCurrent));
    return;
}

void SpectromDebugPage::slot_ReadSpectromNumber()
{
    ui->lineEdit_SerialNumber->setText(SpectromNumber);
    return;
}







