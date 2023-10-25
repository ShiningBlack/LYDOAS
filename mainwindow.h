#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ResolutionCalculationPage.h"
#include "SettingPage.h"
#include "DriftCalculationPage.h"
#include "DebugPage/moduledebugpage.h"
#include "DebugPage/spectromdebugpage.h"

// ui object
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QTextEdit>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QRandomGenerator>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QCheckBox>

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QScrollBar>

// serial port
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

// chart
#include "chart.h"

#include <qwt_math.h>
#include <qwt_spline.h>
#include <qwt_spline_local.h>
#include <qwt_plot_renderer.h>

// thread
#include <QThread>
#include <QtConcurrent/QtConcurrent>

// help
#include <QList>
#include <QFont>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QElapsedTimer>
#include <QVariant>

#include <QDockWidget>
#include <QTableView>
#include <QStandardItemModel>

#include <QDir>
#include <QIntValidator>    // 整数格式器

QT_BEGIN_NAMESPACE
namespace Ui { class LYDOAS; }
QT_END_NAMESPACE

// 主窗口类
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::LYDOAS *ui;

    // 初始化为nullptr非常重要，在后面代码中使用了nullptr做判断，禁止修改！
    ModuleDebugPage           *moduleDebugPage           = nullptr; // 模块调试页
    SpectromDebugPage         *spectromDebugPage         = nullptr; // 光谱调试页
    SettingPage    *preferencesPageGeneral    = nullptr; // 首选项界面
    DriftCalculationPage      *driftCalculationPage      = nullptr; // 漂移量计算
    ResolutionCalculationPage *resolutionCalculationPage = nullptr; // 分辨率计算

    QAction *actionModuleDebug   = nullptr;
    QAction *actionSpectrumDebug = nullptr;

    // serial port
    QTimer           Timer;                         // 连续保存计时器
    QElapsedTimer    m_timer;                       // 计时器

#define BUFFER_SIZE 10240

    QByteArray  buffer; // 存储串口数据的缓冲区
    QSerialPort *serialPort = nullptr;  // 串口对象

    // 强类型枚举类
    enum class Type: unsigned int {
        collType_Default             = 0,
        collType_ModuleAbsorb        = 1,
        collType_ModuleCollecting    = 2,
        collType_SpectrumCollecting  = 3,
        collType_GFCModuleCollecting = 4
    };

    Type type = Type::collType_Default;
    Shiny::Axis axis = {0, 2050, 50, 0, 66000, 4000};

#define FRAME_HEAD_LENGTH       1
#define FRAME_TAIL_LENGTH       1
#define SELFFRAME_HEAD_LENGTH   4
#define SELFFRAME_TAIL_LENGTH   4

    const QByteArray MFrame_Head  = QByteArray::fromHex("F0");
    const QByteArray MFrame_Tail  = QByteArray::fromHex("FF");
    const QByteArray SFrame_Head  = QByteArray::fromHex("011A0115");
    const QByteArray SFrame_Tail  = QByteArray::fromHex("001A0015");

    // chart
    Shiny::Chart *chart;
    QList<QwtPlotCurve *>   listQwtPlotCurve;       // 曲线列表
    QList<QwtPlotCurve *>   listQwtPlotReferCurve;  // 参考曲线列表

    // 最大采集Curve数
    int MaxCurveNum = 51;

    bool m_Flag_Import_Back = false;                // 是否导入背景光谱标识位
    bool m_Flag_Import_Dark = false;                // 是否导入暗光谱标识位
    QVector<double> m_Array_Back_Excel;             // 存储背景光谱数据的数组
    QVector<double> m_Array_Dark_Excel;             // 存储暗光谱数据的数组
    QVector<double> m_Array_Back_After;             // 扣除当前暗电流之后的背景光谱

    // 全局参数
    QFile   *debugFile = nullptr;
    QString ApplicationPath;    // 当前目录
    QString GlobalStoragePath;  // 全局存储目录：未使用

    const QString StrMainWindowsTitle   {"LYDOAS"};
    const QString StrNull               {"null"};
    const QString StrSoftwareNumber     {"软件版本号："};
    const QString StrSpectrumTemper     {"光谱仪温度："};
    const QString StrChamberPressure    {"气室压力："};
    const QString StrChamberTemper      {"气室温度："};
    const QString StrSpectrumNumber     {"序列号："};
    const QString StrSerialPortNoOpen   {"串口未连接"};
    const QString StrSerialPortIsOpen   {"串口连接到："};
    const QString StrSpectrumCount      {"当前光谱数："};
    const QString StrRelativeDrift      {"相对漂移量："};
    const QString StrGetSpectrumTime    {"采集间隔时间："};

    // private: ui object
    QStatusBar  *statusbar;                         // 底栏
    QLabel      *labelSerialOpen;                   // 底栏->是否连接到串口
    QLabel      *labelSpectromNumber;               // 底栏->光谱仪序列号
    QLabel      *labelSoftwareNumber;               // 底栏->软件版本号
    QLabel      *labelSpectromTemper;               // 底栏->光谱仪温度
    QLabel      *labelChamberPressure;              // 底栏->气室压力
    QLabel      *labelChamberTemper;                // 底栏->气室温度
    QLabel      *labelSpectrumCount;                // 底栏->当前光谱数
    QLabel      *labelSpectrumIntervals;            // 底栏->采集间隔

    QAction *actionVisibleConfig;
    QAction *actionVisibleDebug1;
    QAction *actionVisibleDebug2;

    QDockWidget* dockDebugInfo;                     // 调试信息dockwidget
    QDockWidget* dockConfigure;                     // 配置信息dockwidget
    QDockWidget* dockTableView;                     //
    QDockWidget* dockDebugPage;

    QComboBox*   cboxPortName;
    QComboBox*   cboxPortMode;
    QPushButton* btnStartCollect;

    QLineEdit   *lineEdit_MaxCurve;
    QPushButton *btnSetSeriseNum;

    QLineEdit   *lineEdit_ConStorageTime;
    QPushButton *btnStartConExport;

    QCheckBox *cBoxAddTimestamp;
    QCheckBox *cBoxAddFilePrefix;
    QCheckBox *cboxReceiveToFile;
    QLineEdit *lineEdit_FilePrefix;
    QPushButton *btnClearDataAndDebug;
    QPushButton *btnExportDataAndDebug;

    QPlainTextEdit* plainTextEdit_DebugInformation; // 调试信息输出窗口

    QTableView* tableView;
    QStandardItemModel* tableModel;

    /* 私有成员函数 */
    void uiMainInitialize();                // 初始化主窗口
    void uiMenuBarInitialize();             // 初始化菜单栏
    void uiStatusBarInitialize();           // 初始化状态栏
    void uiSubWindowInitialize();           // 初始化子窗口

    // 导出数据到txt文件
    bool exportDataToTxt(const QString &txtData, const QString &txtName);

    // 导出数据到Xlsx文件
    template<typename T>
    bool exportDataToXlsx(const QList<QList<T>> &xlsxData, const QString &xlsxName);
    template<typename T>
    bool exportDataToXlsx(const QVector<QVector<T>> &xlsxData, const QString &xlsxName);

    // 用于构建文件名的函数
    [[nodiscard]] QString constructFileName(const QString prefix, const QString suffix, bool addTimestampFlag = true);

    // 检查一组curve数据是不是暗电流：用于查找暗电流
    [[nodiscard]] bool checkDataFluctuation(const QVector<double>& data);

    // 用于解除模块数据帧中转义字符的函数
    [[nodiscard]] QByteArray Unescaped(const QByteArray &frame, const int length);


    // 用于截取数据帧
    void extractAndProcessSerialFrames(QByteArray &buffer, const QByteArray &head, const QByteArray &tail, const int headLength, const int tailLength);

    // 用于处理截取后的数据帧
    void processModuleFrameData(const QByteArray &frame);
    void processSpectrumFrameData(const QByteArray &frame);

    // 删除多余曲线
    int removeExcessCurves(Shiny::Chart *chart, int maxCurves);

protected: 
    virtual void moveEvent(QMoveEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void signalRefrenceSpectrumImported(QVector<double>*);  // 导入参考光谱信号
    void signalSendCurrentSerialPortData(QVector<double>);  // 发送当前光谱数据信号
    void signalClearInformationPage();                      // 清空调试页信号

private slots:
    // 创建一个分析数据的QChart
    void slotCreateChart(const QList<double> &list, const QStringList &time, const QString header);

    /* 显示TableView的行列右键菜单 */
    void showRowContextMenu(const QPoint &pos);
    void showColContextMenu(const QPoint &pos);

    void slotStartOrStopCollecting();               // 开始采集
    void slotReadAllSerialPortData();               // 读取所有串口数据

    // 菜单栏->保存
    void slotEnableContinuousSave(void);            // 连续保存光谱数据
    void slotExportDebugInfomation(void);           // 导出调试信息为txt文件
    void slotExportSpectralDataToXlsx(void);        // 导出光谱数据为Xlsx文件
    void slotExportSpectrumAndDebugData(void);      // 导出光谱和调试数据
    void slotExportDebugInforToExistFile(void);     // 导出调试数据到文件
    void exportCurvesToPixmap();                    // 导出为图片

    // 菜单栏->导入
    void slotImportBackSpectrum(void);              // 导入背景光谱
    void slotImportRefeSpectrum(void);              // 导入参考光谱

    // 菜单栏->视图
    void slotResetQwtPlotAxis(void);            // 重置QwtPlot坐标轴
    void slotClearQwtPlotCurve(void);           // 清空QwtPlot中的Curve
    void slotClearQwtPlotReferCurve(void);      // 清空QwtPlot中的Refer Curve

    // 菜单栏->工具
    void slotOpenModuleDebugPage(void);         // 打开模块调试页
    void slotOpenSpectrumDebugPage(void);       // 菜单栏->工具->光谱调试：打开光谱调试界面
    void slotOpenResolutionCalculPage(void);    // 打开分辨率计算页
    void slotOpenDriftCalculPage(void);         // 打开偏移量计算页

    // 菜单栏->设置
    void slotOpenFirstSettingPage(void);        // 菜单栏->设置->首选项：打开首选项界面
};

/**
 * @brief  用于放置数据图表的Widget，最大的区别是可以在关闭时销毁自己，达到节省内存的目的
 * @author 2282669851@qq.com
 * @date   2023/10/12
 */
class ChartWidget : public QWidget {
    Q_OBJECT

public:
    ChartWidget(QWidget* parent = nullptr) : QWidget(parent), chart(new QtCharts::QChart()) {
        setWindowTitle("数据分析");
        setWindowFlags(windowFlags() & ~Qt::WindowMinMaxButtonsHint & ~Qt::WindowMaximizeButtonHint);

        chartView = new QtCharts::QChartView(chart, this);  // chart的所有权会被转移到chartView
        chartView->setRenderHint(QPainter::Antialiasing);   // 启用抗锯齿，使图表更加平滑

        xAxisTime = new QtCharts::QDateTimeAxis(chartView);
        xAxisTime->setFormat("hh:mm:ss");
        xAxisTime->setTickCount(10);
        xAxisTime->setLabelsAngle(45);

        yAxisData = new QtCharts::QValueAxis(chartView);
        yAxisData->setTickCount(10);

        chart->addAxis(yAxisData, Qt::AlignLeft);
        chart->addAxis(xAxisTime, Qt::AlignBottom);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(chartView);

        chart->legend()->hide();
    }

    void setChartData(const QList<double>& list, const QList<QDateTime> time, const QString& header) {
        QtCharts::QLineSeries* series = new QtCharts::QLineSeries(chartView);
        for (int i = 0; i < list.size(); ++i) {
            series->append(time.at(i).toMSecsSinceEpoch(), list.at(i));
        }
        chart->addSeries(series);
        chart->setTitle(header);

        series->attachAxis(xAxisTime);
        series->attachAxis(yAxisData);
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        delete this;    // 在关闭事件发生时释放自身内存
        event->accept();
    }

private:
    QtCharts::QChart* chart;
    QtCharts::QChartView* chartView;
    QtCharts::QDateTimeAxis* xAxisTime;
    QtCharts::QValueAxis *yAxisData;
};
#endif // MAINWINDOW_H
