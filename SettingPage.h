#ifndef SETTINGPAGE_H
#define SETTINGPAGE_H
#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QFileDialog>
#include <QSettings>

// 验证器
#include <QIntValidator>

namespace Ui {
class SettingPage;
}

class SettingPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingPage(QWidget *parent = nullptr);
    ~SettingPage();

    QString defaultStoragePath;
    QSerialPort::BaudRate baudRate;
    QSerialPort::DataBits dataBits;
    QSerialPort::StopBits stopBits;
    QSerialPort::Parity parityBits;
    quint32 seriesNumberMax = 51; // 最大折线数

private:
    Ui::SettingPage *ui;

    const QString StrNoParity   { "无" };
    const QString StrOddParity  { "奇" };
    const QString StrEvenParity { "偶" };

private slots:

    void onOptionsChanged();        // 选项更改槽函数
    void slotApplyPushButton();     // <应用> 按钮槽函数
    void onSelectStoragePath();     // 设置默认存储路径
};

#endif
