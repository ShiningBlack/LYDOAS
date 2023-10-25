#include "qserialport.h"
#include "SettingPage.h"
#include "ui_SettingPage.h"

SettingPage::SettingPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingPage)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowTitle(QString("设置"));
    setWindowIcon(QIcon(":/new/prefix1/setting.png"));
    setWindowFlags(this->windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint); // 去掉configui界面的最小化和最大化按钮
    ui->pushButton_Apply->setDisabled(true); // 关闭应用按钮

    // 串口配置信息
    QStringList baudRateList = {"512000", "460800", "256000", "230400", "115200", "57600", "38400", "19200", "9600", "4800", "2400", "1200"};
    ui->comboBox_Baud->addItems(baudRateList);

    QStringList dataBitsList = { "8", "7", "6", "5" };
    ui->comboBox_Data->addItems(dataBitsList);

    QStringList stopBitsList = {"1", "2"};
    ui->comboBox_Stop->addItems(stopBitsList);

    QStringList parityList = { "无", "奇", "偶" };
    ui->comboBox_Parity->addItems(parityList);

    // 设置comboBox的默认选项
    ui->comboBox_Baud->setCurrentText("115200");
    ui->comboBox_Data->setCurrentText("8");
    ui->comboBox_Stop->setCurrentText("1");
    ui->comboBox_Parity->setCurrentText("无");

    // 设置默认参数
    baudRate   = QSerialPort::Baud115200;
    dataBits   = QSerialPort::Data8;
    stopBits   = QSerialPort::OneStop;
    parityBits = QSerialPort::NoParity;

    QObject::connect(ui->comboBox_Baud,   SIGNAL(currentIndexChanged(int)), this, SLOT(onOptionsChanged()));
    QObject::connect(ui->comboBox_Data,   SIGNAL(currentIndexChanged(int)), this, SLOT(onOptionsChanged()));
    QObject::connect(ui->comboBox_Stop,   SIGNAL(currentIndexChanged(int)), this, SLOT(onOptionsChanged()));
    QObject::connect(ui->comboBox_Parity, SIGNAL(currentIndexChanged(int)), this, SLOT(onOptionsChanged()));

    QObject::connect(ui->pushButton_Apply, &QPushButton::clicked, this, &SettingPage::slotApplyPushButton);
    QObject::connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &SettingPage::close);
    QObject::connect(ui->btnSelectStoragePath, &QPushButton::clicked, this, &SettingPage::onSelectStoragePath);

    QSettings settings;
    defaultStoragePath = settings.value("DefaultStoragePath", QVariant(QDir::currentPath())).toString();
    ui->lineEdit_StoragePath->setText(defaultStoragePath);
}

SettingPage::~SettingPage()
{
    delete ui;
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/3
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void SettingPage::onOptionsChanged()
{
    if (!ui->pushButton_Apply->isEnabled()){
        this->ui->pushButton_Apply->setDisabled(false);
    }

    return;
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/3
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void SettingPage::slotApplyPushButton()
{
    this->ui->pushButton_Apply->setDisabled(true);

    baudRate = static_cast<QSerialPort::BaudRate>(ui->comboBox_Baud->currentText().toInt());
    dataBits = static_cast<QSerialPort::DataBits>(ui->comboBox_Data->currentText().toInt());
    stopBits = static_cast<QSerialPort::StopBits>(ui->comboBox_Stop->currentText().toInt());
    if (ui->comboBox_Parity->currentText() == StrNoParity){
        parityBits = QSerialPort::NoParity;
    }else if (ui->comboBox_Parity->currentText() == StrOddParity){
        parityBits = QSerialPort::OddParity;
    }else if (ui->comboBox_Parity->currentText() == StrEvenParity){
        parityBits = QSerialPort::EvenParity;
    }

    // 更新默认存储路径变量
    defaultStoragePath = ui->lineEdit_StoragePath->text();
    QSettings settings;
    settings.setValue("DefaultStoragePath", defaultStoragePath);
    return;
}

void SettingPage::onSelectStoragePath()
{
    // 打开文件对话框以选择存储路径
    QString selectedPath = QFileDialog::getExistingDirectory(this, "选择存储路径", defaultStoragePath);

    // 检查是否选择了路径
    if (!selectedPath.isEmpty()) {
        // 将存储路径显示在 QLineEdit 控件中
        ui->lineEdit_StoragePath->setText(selectedPath);
    }

    // 使能应用按钮
    if (!ui->pushButton_Apply->isEnabled()){
        this->ui->pushButton_Apply->setDisabled(false);
    }
}
