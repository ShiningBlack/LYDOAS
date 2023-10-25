#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_moduledebugpage.h"
#include "ui_spectromdebugpage.h"
#include "xlsxdocument.h"

#ifdef PERFORMANCE_TESTING
#include <chrono>
#endif

QXLSX_USE_NAMESPACE

extern quint16 CRC16(const QByteArray &frame, const quint16 startIndex, const quint16 endIndex);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::LYDOAS)
{
    ui->setupUi(this);
    setWindowTitle(StrMainWindowsTitle);
    setWindowIcon(QIcon(":/main/main.ico"));

    // 主窗口UI
    uiMainInitialize();             // 初始化主窗口
    uiMenuBarInitialize();          // 初始化菜单栏
    uiStatusBarInitialize();        // 初始化状态栏
    uiSubWindowInitialize();        // 初始化子窗口

    // 核心对象
    m_Array_Back_Excel.resize(2048);
    m_Array_Back_After.resize(2048);
    m_Array_Dark_Excel.resize(2048);
    ApplicationPath = QCoreApplication::applicationDirPath() + '/';

    /* 读取串口相关 */
    buffer.reserve(BUFFER_SIZE);
    serialPort = new QSerialPort(this);
    serialPort->setReadBufferSize(BUFFER_SIZE);
}

MainWindow::~MainWindow()
{
    serialPort->close();
    serialPort->disconnect();
    delete serialPort;
    delete ui;
}

/*************************************************
 * 函数类别：私有成员函数
 * 函数功能：初始化主窗口中的对象
 * 函数参数：null
 * 返回对象：null
 * 链接对象：null
 * 修改时间：2023/10/17
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::uiMainInitialize()
{
    // 初始化Qwt折线图
    chart = new Shiny::Chart(this);
    this->setCentralWidget(chart);


    /*-----------------------------------  dockTableView UI布局  ----------------------------------------*/
    dockTableView = new QDockWidget(QString::fromUtf8("调试数据"), this);
    dockTableView->setAllowedAreas(Qt::BottomDockWidgetArea   | Qt::TopDockWidgetArea);
    dockTableView->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    dockTableView->installEventFilter(this);

    tableView  = new QTableView(this);
    tableModel = new QStandardItemModel(this);

    tableView->setModel(tableModel);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);  // 表头拉伸填满可用空间

    QStringList table_h_headers;
    table_h_headers << "模块状态"
                    << "气室压力"
                    << "气室温度"
                    << "光谱仪温度"
                    << "波长漂移量"
                    << "SO2_mg"
                    << "NO2_mg"
                    << "NO_mg"
                    << "H2S_mg"
                    << "NH3_mg"
                    << "SO2_ppm"
                    << "NO2_ppm"
                    << "NO_ppm"
                    << "H2S_ppm"
                    << "NH3_ppm"
                    << "时间";
    tableModel->setHorizontalHeaderLabels(table_h_headers); // 设置水平头部标签

    QHeaderView* headerCol = tableView->horizontalHeader();
    QHeaderView* headerRow = tableView->verticalHeader();

    headerCol->setSectionsMovable(true);
    headerRow->setSectionsMovable(false);

    headerRow->setSectionsClickable(true);
    headerCol->setSectionsClickable(true);

    headerRow->setContextMenuPolicy(Qt::CustomContextMenu);
    headerCol->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(headerRow, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showRowContextMenu(QPoint)));
    connect(headerCol, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showColContextMenu(QPoint)));

    dockTableView->setWidget(tableView);
    addDockWidget(Qt::BottomDockWidgetArea, dockTableView);


    /*-----------------------------------  dockDebugInfo UI布局  ----------------------------------------*/
    dockDebugInfo = new QDockWidget(QString::fromUtf8("原始数据"), this);
    dockDebugInfo->setAllowedAreas(Qt::BottomDockWidgetArea   | Qt::TopDockWidgetArea);
    dockDebugInfo->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    dockDebugInfo->installEventFilter(this);

    QWidget *winDebugInfo = new QWidget(dockDebugInfo);

    QVBoxLayout *vlDebugInfo;
    vlDebugInfo = new QVBoxLayout(winDebugInfo);

    QHBoxLayout *hlDebugInfo;
    hlDebugInfo = new QHBoxLayout();

    cBoxAddTimestamp  = new QCheckBox(QString::fromUtf8("添加时间戳"), winDebugInfo);
    cboxReceiveToFile = new QCheckBox(QString::fromUtf8("接收到文件"), winDebugInfo);
    hlDebugInfo->addWidget(cBoxAddTimestamp,  0, Qt::AlignLeft);
    hlDebugInfo->addWidget(cboxReceiveToFile, 0, Qt::AlignLeft);

    QSpacerItem *hSpacer;
    hSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hlDebugInfo->addItem(hSpacer);

    plainTextEdit_DebugInformation = new QPlainTextEdit(dockDebugInfo); // 调试信息输出栏
    plainTextEdit_DebugInformation->setObjectName(QString::fromUtf8("plainTextEdit_DebugInformation"));
    plainTextEdit_DebugInformation->setReadOnly(true);
    plainTextEdit_DebugInformation->setMinimumSize(QSize(0, 0));
    plainTextEdit_DebugInformation->setMaximumSize(QSize(16777215, 16777215));

    vlDebugInfo->addLayout(hlDebugInfo);
    vlDebugInfo->addWidget(plainTextEdit_DebugInformation);

    dockDebugInfo->setWidget(winDebugInfo);
    addDockWidget(Qt::BottomDockWidgetArea, dockDebugInfo);

    tabifyDockWidget(dockDebugInfo, dockTableView);
//    splitDockWidget(dockTableView, dockDebugInfo, Qt::Vertical);


    /*-----------------------------------  dockConfigure UI布局  ----------------------------------------*/
    dockConfigure = new QDockWidget(QString::fromUtf8("配置信息"), this);
    dockConfigure->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    dockConfigure->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    dockConfigure->installEventFilter(this);

    QWidget* widget;
    widget = new QWidget(dockConfigure);

    /* 串口采集 */
    QGridLayout* gridLayout;
    gridLayout = new QGridLayout(widget);

    QGroupBox*   gboxPortInfo;
    gboxPortInfo = new QGroupBox("串口采集", widget);

    QVBoxLayout *vboxPortInfo;
    vboxPortInfo = new QVBoxLayout(gboxPortInfo);

    cboxPortName = new QComboBox(gboxPortInfo);
    cboxPortMode = new QComboBox(gboxPortInfo);
    btnStartCollect = new QPushButton(QString::fromUtf8("开始采集"), gboxPortInfo);

    vboxPortInfo->addWidget(cboxPortName);
    vboxPortInfo->addWidget(cboxPortMode);
    vboxPortInfo->addWidget(btnStartCollect);

    gridLayout->addWidget(gboxPortInfo, 0, 0, 1, 1);

    /* 曲线数 */
    QGroupBox *gboxSeriseNum;
    gboxSeriseNum = new QGroupBox("最大折线数", widget);

    QVBoxLayout *vboxSeriseNum = new QVBoxLayout(gboxSeriseNum);
    QHBoxLayout *hboxSeriseNum = new QHBoxLayout;

    QLabel *labelSeriseNum;
    labelSeriseNum    = new QLabel("折线数:", gboxSeriseNum);
    lineEdit_MaxCurve = new QLineEdit(gboxSeriseNum);

    hboxSeriseNum->addWidget(labelSeriseNum);
    hboxSeriseNum->addWidget(lineEdit_MaxCurve);
    hboxSeriseNum->setStretch(0, 4);
    hboxSeriseNum->setStretch(1, 6);

    btnSetSeriseNum = new QPushButton("设置", gboxSeriseNum);

    vboxSeriseNum->addLayout(hboxSeriseNum);
    vboxSeriseNum->addWidget(btnSetSeriseNum);

    gridLayout->addWidget(gboxSeriseNum, 1, 0, 1, 1);

    /* 间隔保存 */
    QGroupBox *gboxConExport;
    gboxConExport = new QGroupBox("间隔保存", widget);

    QVBoxLayout *vboxConExport;
    vboxConExport = new QVBoxLayout(gboxConExport);

    QHBoxLayout *hboxConExport = new QHBoxLayout;
    QLabel *labIntervals;
    labIntervals = new QLabel("间隔时间:", gboxConExport);
    lineEdit_ConStorageTime = new QLineEdit(gboxConExport);

    hboxConExport->addWidget(labIntervals);
    hboxConExport->addWidget(lineEdit_ConStorageTime);
    hboxConExport->setStretch(0, 4);
    hboxConExport->setStretch(1, 6);

    btnStartConExport = new QPushButton(QString::fromUtf8("开始间隔保存"), gboxConExport);

    vboxConExport->addLayout(hboxConExport);
    vboxConExport->addWidget(btnStartConExport);

    gridLayout->addWidget(gboxConExport, 2, 0, 1, 1);

    /* 快速调试 */
    QGroupBox *gboxQuickDebug;
    gboxQuickDebug = new QGroupBox("快速保存", widget);

    QVBoxLayout *vboxQuickDebug = new QVBoxLayout(gboxQuickDebug);

    cBoxAddFilePrefix     = new QCheckBox("添加文件名前缀", gboxQuickDebug);
    lineEdit_FilePrefix   = new QLineEdit(gboxQuickDebug);
    btnClearDataAndDebug  = new QPushButton("清空光谱和调试数据", gboxQuickDebug);
    btnExportDataAndDebug = new QPushButton("保存光谱和调试数据", gboxQuickDebug);

    vboxQuickDebug->addWidget(cBoxAddFilePrefix);
    vboxQuickDebug->addWidget(lineEdit_FilePrefix);
    vboxQuickDebug->addWidget(btnClearDataAndDebug);
    vboxQuickDebug->addWidget(btnExportDataAndDebug);

    gridLayout->addWidget(gboxQuickDebug, 3, 0, 1, 1);

    /* 小弹簧 */
    QSpacerItem *vSpacer;
    vSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(vSpacer, 4, 0, 1, 1);

    dockConfigure->setWidget(widget);
    addDockWidget(Qt::LeftDockWidgetArea, dockConfigure);


    /*-----------------------------------  dockConfigure 逻辑  ------------------------------------------*/
    cboxPortName->installEventFilter(this);

    // 将可用串口调添加到串口列表
    QList<QSerialPortInfo> list_Serial = QSerialPortInfo::availablePorts();
    for (const auto &item : list_Serial) {
        cboxPortName->addItem(item.portName() + " " + item.description());
    }

    // 讲采集模式添加到模式列表
    QStringList modeList = {"采集光谱仪模块", "采集光谱仪", "采集光谱仪模块吸光度"};
    cboxPortMode->addItems(modeList);

    // 最大折线数
    QIntValidator *validator1 = new QIntValidator(0, 999, this);
    lineEdit_MaxCurve->setValidator(validator1);
    lineEdit_MaxCurve->setText(QString::number(MaxCurveNum));
    QObject::connect(btnSetSeriseNum, &QPushButton::clicked, this, [=](){

        MaxCurveNum = lineEdit_MaxCurve->text().toInt();
        QMessageBox::information(this, StrMainWindowsTitle, "设置成功");
    });

    // 间隔时间保存
    QIntValidator *validator2 = new QIntValidator(0, 99999, this);
    lineEdit_ConStorageTime->setValidator(validator2);
    lineEdit_ConStorageTime->setText("10");

    QObject::connect(btnStartCollect, &QPushButton::clicked, this, &MainWindow::slotStartOrStopCollecting);     // 链接开始采集槽函数
    QObject::connect(btnStartConExport, &QPushButton::clicked, this, &MainWindow::slotEnableContinuousSave);    // 连续保存光谱数据

    // 清空调试信息窗口
    auto clearPlainEdit = [this](){ plainTextEdit_DebugInformation->clear(); };
    QObject::connect(btnClearDataAndDebug,  &QPushButton::clicked, this, clearPlainEdit);
    QObject::connect(btnClearDataAndDebug,  &QPushButton::clicked, this, &MainWindow::slotClearQwtPlotCurve);       // 清空光谱数据

    /* 调试区 */
    QObject::connect(cboxReceiveToFile, &QCheckBox::stateChanged,  this, &MainWindow::slotExportDebugInforToExistFile); // 接收数据到文件
    QObject::connect(btnExportDataAndDebug, &QPushButton::clicked, this, &MainWindow::slotExportSpectrumAndDebugData);  // 导出光谱数据和调试信息
}

/**
 * @brief MainWindow::uiMenuBarInitialize
 * @date  2023/7/3
 * @note  初始化MainWindow的菜单栏
 */
void MainWindow::uiMenuBarInitialize()
{
    // 菜单栏
    QMenuBar *menubar = menuBar();;

    // 菜单栏->保存
    QMenu   *menuSave = new QMenu("导出", this);
    QAction *actionSaveSpectrum  = new QAction(QIcon(":/icon/Icon/save2.png"), "导出光谱数据", menuSave);
    QAction *actionSaveDebugInfo = new QAction(QIcon(":/icon/Icon/save2.png"), "导出调试信息", menuSave);
    QAction *actionSaveToPixmap  = new QAction(QIcon(":/icon/Icon/exportToPixmap.png"), "导出光谱图", menuSave);

    QObject::connect(actionSaveSpectrum,  &QAction::triggered, this, &MainWindow::slotExportSpectralDataToXlsx);  // 导出光谱数据到Xlsx文件
    QObject::connect(actionSaveDebugInfo, &QAction::triggered, this, &MainWindow::slotExportDebugInfomation);     // 导出调试数据到txt文件
    QObject::connect(actionSaveToPixmap,  &QAction::triggered, this, &MainWindow::exportCurvesToPixmap);          // 导出折线图图像

    menuSave->addAction(actionSaveSpectrum);
    menuSave->addAction(actionSaveDebugInfo);
    menuSave->addAction(actionSaveToPixmap);
    menubar->addMenu(menuSave);

    // 菜单栏->导入
    QMenu   *menuImport = new QMenu("导入", this);
    QAction *actionImportRefeSpectrum = new QAction(QIcon(":/icon/Icon/import_dark.png"), "导入参考光谱", menuImport);
    QAction *actionImportBackSpectrum = new QAction(QIcon(":/icon/Icon/import_dark.png"), "导入背景光谱", menuImport);

    QObject::connect(actionImportBackSpectrum, &QAction::triggered, this, &MainWindow::slotImportBackSpectrum);    // 导入背景光谱
    QObject::connect(actionImportRefeSpectrum, &QAction::triggered, this, &MainWindow::slotImportRefeSpectrum);    // 导入参考光谱

    menuImport->addAction(actionImportRefeSpectrum);
    menuImport->addAction(actionImportBackSpectrum);
    menubar->addMenu(menuImport);

    // 菜单栏->视图->UI
    QMenu   *menuView = new QMenu("视图", this);
    QAction *actionViewReset       = new QAction(QIcon(":/icon/Icon/resetAxis.png"), "重置坐标轴", menuView);
    QAction *actionClearChart      = new QAction(QIcon(":/icon/Icon/clear.png"), "清空光谱曲线", menuView);
    QAction *actionClearReferCurve = new QAction(QIcon(":/icon/Icon/clear.png"), "清除参考曲线", menuView);
    QAction *actionClearDebugInfor = new QAction(QIcon(":/icon/Icon/clear.png"), "清空调试数据", menuView);

    actionVisibleConfig    = new QAction(QString::fromUtf8("配置信息"), menuView);
    actionVisibleDebug1    = new QAction(QString::fromUtf8("调试数据分析"), menuView);
    actionVisibleDebug2    = new QAction(QString::fromUtf8("原始调试数据"), menuView);
    actionVisibleConfig->setCheckable(true);
    actionVisibleDebug1->setCheckable(true);
    actionVisibleDebug2->setCheckable(true);
    actionVisibleConfig->setChecked(true);
    actionVisibleDebug1->setChecked(true);
    actionVisibleDebug2->setChecked(true);

    menuView->addAction(actionViewReset);
    menuView->addAction(actionClearChart);
    menuView->addAction(actionClearReferCurve);
    menuView->addAction(actionClearDebugInfor);

    menuView->addAction(actionVisibleConfig);
    menuView->addAction(actionVisibleDebug1);
    menuView->addAction(actionVisibleDebug2);
    menubar->addMenu(menuView);

    // 菜单栏->视图->逻辑
    QObject::connect(actionViewReset,       &QAction::triggered, this, &MainWindow::slotResetQwtPlotAxis);          // 重置坐标轴
    QObject::connect(actionClearChart,      &QAction::triggered, this, &MainWindow::slotClearQwtPlotCurve);         // 清空曲线
    QObject::connect(actionClearReferCurve, &QAction::triggered, this, &MainWindow::slotClearQwtPlotReferCurve);    // 清除参考曲线

    auto lambdaClearDebugInfo = [=](){ plainTextEdit_DebugInformation->clear(); };
    QObject::connect(actionClearDebugInfor, &QAction::triggered, this, lambdaClearDebugInfo);                       // 清空调试数据

    auto toggleConfig = [=](bool checked){ dockConfigure->setVisible(checked); };
    QObject::connect(actionVisibleConfig, &QAction::triggered, this, toggleConfig);

    auto toggleDebug1 = [=](bool checked){ dockTableView->setVisible(checked); };
    auto toggleDebug2 = [=](bool checked){ dockDebugInfo->setVisible(checked); };
    QObject::connect(actionVisibleDebug1, &QAction::triggered, this, toggleDebug1);
    QObject::connect(actionVisibleDebug2, &QAction::triggered, this, toggleDebug2);

    // 菜单栏->图表
    QMenu   *menuChart = new QMenu("图表", this);
    QMenu   *menuAxis  = new QMenu(QString::fromUtf8("坐标轴"), menuChart);
    QAction *actionShowXTop    = new QAction(QString::fromUtf8("X-Top"),    menuAxis);
    QAction *actionShowXBotton = new QAction(QString::fromUtf8("X-Botton"), menuAxis);
    QAction *actionShowYLeft   = new QAction(QString::fromUtf8("Y-Left"),   menuAxis);
    QAction *actionShowYRight  = new QAction(QString::fromUtf8("Y-Right"),  menuAxis);
    QAction *actionMarkerAble  = new QAction(QString::fromUtf8("使用标记"),   menuChart);
    QAction *actionClearMarker = new QAction(QIcon(":/icon/Icon/clear.png"), QString::fromUtf8("清除标记"), menuChart);

    actionMarkerAble-> setCheckable(true);
    actionShowYLeft->  setCheckable(true);
    actionShowXBotton->setCheckable(true);
    actionShowXTop->   setCheckable(true);
    actionShowYRight-> setCheckable(true);

    actionShowYLeft->  setChecked(true);
    actionShowXBotton->setChecked(true);

    QMenu *menuGrid = new QMenu(QString::fromUtf8("网格线"), menuChart);
    QAction *actionShowGrid  = new QAction(QString::fromUtf8("开关"),  menuGrid);
    QAction *actionSolidLine = new QAction(QString::fromUtf8("实线"),  menuGrid);
    QAction *actionDotLine   = new QAction(QString::fromUtf8("点线"),  menuGrid);
    QAction *actionDashLine  = new QAction(QString::fromUtf8("虚线"),  menuGrid);
    QAction *actionDDotLine  = new QAction(QString::fromUtf8("点划线"), menuGrid);
    actionShowGrid->setCheckable(true);
    actionShowGrid->setChecked(true);

    auto setSolidLine = [this](){ chart->setGridLineStyle(Qt::SolidLine);   };
    auto setDotLine   = [this](){ chart->setGridLineStyle(Qt::DotLine);     };
    auto setDashLine  = [this](){ chart->setGridLineStyle(Qt::DashLine);    };
    auto setDDotLine  = [this](){ chart->setGridLineStyle(Qt::DashDotLine); };
    connect(actionSolidLine, &QAction::triggered, this, setSolidLine);
    connect(actionDotLine,   &QAction::triggered, this, setDotLine);
    connect(actionDashLine,  &QAction::triggered, this, setDashLine);
    connect(actionDDotLine,  &QAction::triggered, this, setDDotLine);

    menuGrid->addAction(actionShowGrid);
    menuGrid->addAction(actionSolidLine);
    menuGrid->addAction(actionDotLine);
    menuGrid->addAction(actionDashLine);
    menuGrid->addAction(actionDDotLine);

    auto lambdaShowAxis = [=](){
        QAction* action = qobject_cast<QAction*>(QObject::sender());

        if (action == actionShowYLeft)
            chart->enableAxis(QwtAxis::YLeft,   action->isChecked());
        else if (action == actionShowXBotton)
            chart->enableAxis(QwtAxis::XBottom, action->isChecked());
        else if (action == actionShowXTop)
            chart->enableAxis(QwtAxis::XTop,    action->isChecked());
        else
            chart->enableAxis(QwtAxis::YRight,  action->isChecked());
    };

    QObject::connect(actionShowYLeft,   &QAction::triggered, this, lambdaShowAxis);
    QObject::connect(actionShowXBotton, &QAction::triggered, this, lambdaShowAxis);
    QObject::connect(actionShowXTop,    &QAction::triggered, this, lambdaShowAxis);
    QObject::connect(actionShowYRight,  &QAction::triggered, this, lambdaShowAxis);

    QObject::connect(actionShowGrid,    &QAction::triggered, chart, &Shiny::Chart::gridSwitch);
    QObject::connect(actionMarkerAble,  &QAction::triggered, chart, &Shiny::Chart::markerSwitch);
    QObject::connect(actionClearMarker, &QAction::triggered, chart, &Shiny::Chart::markerClear);

    menuChart->addMenu(menuAxis);
    menuAxis->addAction(actionShowYLeft);
    menuAxis->addAction(actionShowXBotton);
    menuAxis->addAction(actionShowXTop);
    menuAxis->addAction(actionShowYRight);

    menuChart->addMenu(menuGrid);
    menuChart->addAction(actionMarkerAble);
    menuChart->addAction(actionClearMarker);
    menubar->addMenu(menuChart);

    // 菜单栏->工具
    QMenu   *menuTools = new QMenu("工具", this);
    QAction *actionReolutionDebug = new QAction(QIcon(":/icon/Icon/resolution.png"), "分辨率计算", menuTools);
    QAction *actionDriftDebug     = new QAction(QIcon(":/icon/Icon/wave.png"), "漂移量计算", menuTools);
    actionModuleDebug    = new QAction(QIcon(":/icon/Icon/module.png"), "模块调试", menuTools);
    actionSpectrumDebug  = new QAction(QIcon(":/icon/Icon/debug.png"), "光谱调试", menuTools);

    QObject::connect(actionModuleDebug,    &QAction::triggered, this, &MainWindow::slotOpenModuleDebugPage);        // 打开模块调试页
    QObject::connect(actionSpectrumDebug,  &QAction::triggered, this, &MainWindow::slotOpenSpectrumDebugPage);      // 打开光谱调试页
    QObject::connect(actionReolutionDebug, &QAction::triggered, this, &MainWindow::slotOpenResolutionCalculPage);   // 打开分辨率计算页
    QObject::connect(actionDriftDebug,     &QAction::triggered, this, &MainWindow::slotOpenDriftCalculPage);        // 打开偏移量计算页

    menuTools->addAction(actionModuleDebug);
    menuTools->addAction(actionSpectrumDebug);
    menuTools->addAction(actionReolutionDebug);
    menuTools->addAction(actionDriftDebug);
    menubar->addMenu(menuTools);

    // 菜单栏->设置
    QMenu   *menuSetting = new QMenu("设置", this);
    QAction *actionFirstSetting = new QAction(QIcon(":/icon/Icon/setting.png"), "首选项", menuSetting);
    QObject::connect(actionFirstSetting,   &QAction::triggered, this, &MainWindow::slotOpenFirstSettingPage);       // 打开首选项界面
    menuSetting->addAction(actionFirstSetting);
    menubar->addMenu(menuSetting);

    /* 工具栏 */
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(true);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    toolbar->setStyleSheet("QToolBar { background: white; border: none; }");

    toolbar->addAction(actionImportRefeSpectrum);
    toolbar->addAction(actionImportBackSpectrum);
    toolbar->addSeparator();

    toolbar->addAction(actionViewReset);
    toolbar->addAction(actionClearChart);
    toolbar->addAction(actionClearReferCurve);
    toolbar->addAction(actionClearDebugInfor);
    toolbar->addSeparator();

    toolbar->addAction(actionModuleDebug);
    toolbar->addAction(actionSpectrumDebug);
    toolbar->addAction(actionReolutionDebug);
    toolbar->addAction(actionDriftDebug);
    toolbar->addSeparator();

    toolbar->addAction(actionSaveSpectrum);
    toolbar->addAction(actionSaveDebugInfo);
    toolbar->addAction(actionSaveToPixmap);
    toolbar->addSeparator();

    toolbar->addAction(actionFirstSetting);

    this->addToolBar(toolbar);
}

/*************************************************
 * 函数类别：私有成员函数
 * 函数功能：初始化底栏
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/3
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::uiStatusBarInitialize()
{
    labelSerialOpen         = new QLabel(this);
    labelSpectromNumber     = new QLabel(this);
    labelSoftwareNumber     = new QLabel(this);
    labelSpectromTemper     = new QLabel(this);
    labelChamberPressure    = new QLabel(this);
    labelChamberTemper      = new QLabel(this);
    labelSpectrumCount      = new QLabel(this);
    labelSpectrumIntervals  = new QLabel(this);

    statusbar = this->statusBar();
    statusbar->addWidget(labelSerialOpen);
    statusbar->addWidget(labelSpectromNumber);
    statusbar->addWidget(labelSoftwareNumber);
    statusbar->addWidget(labelSpectromTemper);
    statusbar->addWidget(labelChamberPressure);
    statusbar->addWidget(labelChamberTemper);
    statusbar->addWidget(labelSpectrumCount);
    statusbar->addWidget(labelSpectrumIntervals);

    labelSerialOpen->setText(StrSerialPortNoOpen);
    labelSpectromNumber->setText(StrSpectrumNumber + StrNull);
    labelSoftwareNumber->setText(StrSoftwareNumber + StrNull);
    labelSpectromTemper->setText(StrSpectrumTemper + StrNull);
    labelChamberPressure->setText(StrChamberPressure + StrNull);
    labelChamberTemper->setText(StrChamberTemper + StrNull);
    labelSpectrumCount ->setText(StrSpectrumCount  + StrNull);
    labelSpectrumIntervals -> setText(StrGetSpectrumTime + StrNull);
}

void MainWindow::uiSubWindowInitialize()
{
    // 首选项页
    preferencesPageGeneral = new SettingPage(this);
    preferencesPageGeneral->hide();

    // 偏移量计算页
    driftCalculationPage = new DriftCalculationPage(this);
    QObject::connect(this, &MainWindow::signalSendCurrentSerialPortData, driftCalculationPage, &DriftCalculationPage::slotNewData);
    QObject::connect(this, &MainWindow::signalRefrenceSpectrumImported,  driftCalculationPage, &DriftCalculationPage::slotIsReferenceSpectrumImported);
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：并行保存光谱数据到Xlsx文件
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/8/10
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotExportSpectralDataToXlsx()
{
    // 先把参考曲线列表移除
    for (auto &refer : listQwtPlotReferCurve) {
        refer->detach();
    }

    QList<QList<double>> list = chart->getAllCurvePointY();

    if (list.size() <= 0) {
        QMessageBox::information(this, StrMainWindowsTitle, "没有数据可用于保存Xlsx文件！");
        return;
    }

    // 构建文件名和路径
    QString fileName = constructFileName("Spectrum", ".xlsx", true);

    // 导出数据到Xlsx文件
    if (exportDataToXlsx(list, fileName)) {
        QMessageBox::information(this, StrMainWindowsTitle, "保存Xlsx文件完成：" + fileName);
    } else {
        QMessageBox::warning(this, StrMainWindowsTitle, "保存Xlsx文件失败！");
    }

    // 恢复参考曲线
    for (auto &refer : listQwtPlotReferCurve) {
        refer->attach(chart);
    }
}

/**
 * @brief MainWindow::constructFileName
 * @param prefix  "文件名前缀"
 * @param suffix: "."开头的文件名后缀
 * @param addTimestampFlag: 是否给文件名加上时间戳
 * @return 构建完成的文件名QString副本
 */
QString MainWindow::constructFileName(const QString prefix, const QString suffix, bool addTimestampFlag/* = true*/)
{
    QString storagePath = preferencesPageGeneral->defaultStoragePath + '/';

    if (QDir(storagePath).exists() && !storagePath.isEmpty()) {
        storagePath = QDir::toNativeSeparators(storagePath);
    } else {
        storagePath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + '/');
    }

    QString labelText = labelSpectromNumber->text();
    QString serialNum = labelText.replace(StrSpectrumNumber, "").remove(QChar('\u0000')).replace(' ', "");

    if (addTimestampFlag) {
        const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
        return storagePath + prefix + serialNum + "-" + timestamp + suffix;
    }

    return storagePath + prefix + serialNum + suffix;
}


/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：保存光谱和调试数据
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/19
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotExportSpectrumAndDebugData()
{
    QList<QList<double>> list = chart->getAllCurvePointY();

    if (list.isEmpty() && plainTextEdit_DebugInformation->toPlainText().isEmpty()) {
        QMessageBox::information(this, StrMainWindowsTitle, "没有光谱数据和调试数据！");
        return;
    }

    QString txtName, xlsxName;

    // 判断有没有勾选添加文件名前缀
    if (cBoxAddFilePrefix->isChecked()) {
        if (lineEdit_FilePrefix->text().isEmpty()) {
            QMessageBox::warning(this, StrMainWindowsTitle, "请输入文件名前缀再进行保存！");
            return;
        }
        txtName  = constructFileName(lineEdit_FilePrefix->text(), ".txt",  false);
        xlsxName = constructFileName(lineEdit_FilePrefix->text(), ".xlsx", false);

    } else {
        txtName  = constructFileName("Debug",    ".txt",  true);
        xlsxName = constructFileName("Spectrum", ".xlsx", true);
    }

    // 导出数据
    bool txtExportSuccess  = exportDataToTxt(plainTextEdit_DebugInformation->toPlainText(), txtName);
    bool xlsxExportSuccess = exportDataToXlsx(list, xlsxName);

    if (!txtExportSuccess)
        QMessageBox::information(this, StrMainWindowsTitle, "保存调试数据失败！");

    if (!xlsxExportSuccess)
        QMessageBox::information(this, StrMainWindowsTitle, "保存光谱数据失败！");

    if (txtExportSuccess && xlsxExportSuccess)
        QMessageBox::information(this, StrMainWindowsTitle, "保存成功！");
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/20
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotExportDebugInforToExistFile()
{
    if (cboxReceiveToFile->isChecked()) {
        QString txtName = constructFileName("ReceiveToTxt", ".txt", true);
        debugFile = new QFile(txtName);

        if (!debugFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
            cboxReceiveToFile->setChecked(false);
            return;
        }

        QMessageBox::information(this, StrMainWindowsTitle, "之后输出的调试信息将保存到" + txtName);
    } else {
        debugFile->close();
        debugFile = nullptr;
    }
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/5
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotEnableContinuousSave()
{
    if (!Timer.isActive()) {

        if (type == Type::collType_Default) {
            QMessageBox::information(this, StrMainWindowsTitle, "当前没有采集光谱！");
            return;
        }

        if (lineEdit_ConStorageTime->text().isEmpty()) {
            QMessageBox::information(this, StrMainWindowsTitle, "请输入连续保存时间！");
            return;
        }

        int time = lineEdit_ConStorageTime->text().toInt();
        Timer.start(time * 1000);

        // 连续保存光谱数据信号槽函数
        QObject::connect(&Timer, &QTimer::timeout, this, [=](){
            if (chart->curveIsEmpty())
                return;

            QString xlsxName = constructFileName("Spectrum", ".xlsx", true);

            QList<QList<double>> list = chart->getAllCurvePointY();
            exportDataToXlsx(list, xlsxName);
        });

        btnStartConExport->setText("停止间隔保存");
    }else {
        Timer.stop();
        QObject::disconnect(&Timer, &QTimer::timeout, this, nullptr);   // 断开定时器的信号槽链接

        btnStartConExport->setText("开始间隔保存");
        QMessageBox::information(this, StrMainWindowsTitle, QString("关闭间隔保存成功！"));
    }
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：打开首选项界面
 * 函数参数：null
 * 返回对象：null
 * 链接对象：菜单栏->设置->首选项
 * 修改时间：2023/6/27
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotOpenFirstSettingPage()
{
    // 计算主窗口中心位置的相对坐标
    QPoint mainWindowCenter = rect().center();

    // 将主窗口中心位置的相对坐标转换为屏幕坐标
    QPoint screenMainWindowCenter = mapToGlobal(mainWindowCenter);

    // 计算子窗口在主窗口中心的位置
    int x = screenMainWindowCenter.x() - preferencesPageGeneral->width() / 2;
    int y = screenMainWindowCenter.y() - preferencesPageGeneral->height() / 2;

    // 移动子窗口到计算得到的位置
    preferencesPageGeneral->move(x, y);
    preferencesPageGeneral->show();
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：重置图表坐标轴
 * 函数参数：null
 * 返回对象：null
 * 链接对象：菜单栏->视图->重置
 * 修改时间：2023/9/20
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotResetQwtPlotAxis()
{
    switch (type) {
    case Type::collType_ModuleAbsorb:
        axis = (Shiny::Axis){0, 2050, 50, -4, 2, 1};
        break;
    case Type::collType_GFCModuleCollecting:
        axis = (Shiny::Axis){0, 240, 10, -50000, 66000, 8000};
        break;
    case Type::collType_Default:
    case Type::collType_ModuleCollecting:
    case Type::collType_SpectrumCollecting:
    default:
        axis = (Shiny::Axis){0, 2050, 50, 0, 66000, 4000};
        break;
    }

    chart->setAxis(axis);
    chart->replot();
}

/**
 * @brief 删除所有数据曲线
 * @date  2023/10/16
 */
void MainWindow::slotClearQwtPlotCurve()
{
    for (auto& curve : listQwtPlotCurve) {
        curve->detach();
        delete curve;
    }
    listQwtPlotCurve.clear();
    labelSpectrumCount ->setText(StrSpectrumCount  + StrNull);  // 更新状态栏
    chart->replot();
}

/**
 * @brief 删除所有导入的参考曲线
 * @date 2023/09/16
 */
void MainWindow::slotClearQwtPlotReferCurve()
{
    for (auto& curve : listQwtPlotReferCurve) {
        curve->detach();
        delete curve;
    }
    listQwtPlotReferCurve.clear();
    chart->replot();
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：打开模块调试页
 * 函数参数：null
 * 返回对象：null
 * 链接对象：菜单栏->工具->模块调试
 * 修改时间：2023/10/16
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotOpenModuleDebugPage()
{
    if (!moduleDebugPage) {
        moduleDebugPage = new ModuleDebugPage(this->serialPort, this);
        moduleDebugPage->resetFlag = true;

        auto close = [this](QObject * obj){ moduleDebugPage = nullptr; Q_UNUSED(obj) };
        connect(moduleDebugPage, &QObject::destroyed, this, close);

        moduleDebugPage->show();
    } else {
        moduleDebugPage->close();
    }
}

// 打开光谱仪调试页
void MainWindow::slotOpenSpectrumDebugPage()
{
    if (!spectromDebugPage) {
        spectromDebugPage = new SpectromDebugPage(this->serialPort, this);
        spectromDebugPage->resetFlag = true;

        auto close = [this](QObject * obj){ spectromDebugPage = nullptr; Q_UNUSED(obj) };
        connect(spectromDebugPage, &QObject::destroyed, this, close);

        spectromDebugPage->show();
    } else {
        spectromDebugPage->close();
    }
}

// 打开
void MainWindow::slotOpenResolutionCalculPage()
{
    if (!resolutionCalculationPage) {
        resolutionCalculationPage = new ResolutionCalculationPage(this);

        auto close = [this](QObject * obj){ resolutionCalculationPage = nullptr; Q_UNUSED(obj) };
        connect(resolutionCalculationPage, &QObject::destroyed, this, close);
        connect(this, &MainWindow::signalSendCurrentSerialPortData, resolutionCalculationPage, &ResolutionCalculationPage::slotNewData);

        resolutionCalculationPage->show();
    } else {
        resolutionCalculationPage->close();
    }
}

// 打开漂移量计算页
void MainWindow::slotOpenDriftCalculPage()
{
    if (driftCalculationPage->isVisible()) {
        driftCalculationPage->close();
    } else {
        driftCalculationPage->show();
    }
}

/*************************************************
 * 函数类别：重写事件函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：null
 * 链接对象：主窗口移动事件
 * 修改时间：2023/6/30
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::moveEvent(QMoveEvent *event)
{
    // 调用父类的移动事件处理函数
    QMainWindow::moveEvent(event);

    if (preferencesPageGeneral->isVisible()) {
        // 计算主窗口中心位置的相对坐标
        QPoint mainWindowCenter = rect().center();

        // 将主窗口中心位置的相对坐标转换为屏幕坐标
        QPoint screenMainWindowCenter = mapToGlobal(mainWindowCenter);

        // 计算子窗口在主窗口中心的位置
        int x = screenMainWindowCenter.x() - preferencesPageGeneral->width() / 2;
        int y = screenMainWindowCenter.y() - preferencesPageGeneral->height() / 2;

        // 移动子窗口到计算得到的位置
        preferencesPageGeneral->move(x, y);
    }
}

/**
 * @brief MainWindow::resizeEvent
 * @param event
 * @date  2023/6/30
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (preferencesPageGeneral->isVisible()) {
        // 计算主窗口中心位置的相对坐标
        QPoint mainWindowCenter = rect().center();

        // 将主窗口中心位置的相对坐标转换为屏幕坐标
        QPoint screenMainWindowCenter = mapToGlobal(mainWindowCenter);

        // 计算子窗口在主窗口中心的位置
        int x = screenMainWindowCenter.x() - preferencesPageGeneral->width() / 2;
        int y = screenMainWindowCenter.y() - preferencesPageGeneral->height() / 2;

        // 移动子窗口到计算得到的位置
        preferencesPageGeneral->move(x, y);
    }
}
/**
 * @brief MainWindow::eventFilter
 * @param object
 * @param event
 * @return
 * @date  2023/09/18
 */
bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == cboxPortName && event->type() == QEvent::MouseButtonPress && cboxPortName->isEnabled()) {
        qDebug() << "ecentFilter函数执行！";
        cboxPortName->clear();   // 清除旧的串口列表
        // 添加串口列表到下拉框
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo& port : ports) {
            cboxPortName->addItem(port.portName() + " " + port.description());
        }
    }

    if (object == dockConfigure && event->type() == QEvent::Close) {
        actionVisibleConfig->setChecked(false);
    }
    if (object == dockDebugInfo && event->type() == QEvent::Close) {
        actionVisibleDebug2->setChecked(false);
    }
    if (object == dockTableView && event->type() == QEvent::Close) {
        actionVisibleDebug1->setChecked(false);
    }
    return QMainWindow::eventFilter(object, event);
}

/**
 * @brief 创建一个分析数据的QChart
 * @param list: chart数据
 * @param time: 采集数据的时间list
 * @param header 数据名称，用来作为chart的标题
 * @date  2023/10/16
 */
void MainWindow::slotCreateChart(const QList<double> &list, const QStringList &time, const QString header)
{
    ChartWidget* chartWidget = new ChartWidget(this);
    chartWidget->setWindowFlag(Qt::Window);
    chartWidget->resize(1200, 400);

    QList<QDateTime> listTime;
    for (auto &item : time) {
        listTime.append(QDateTime::fromString(item, "MM-dd-hh:mm:ss"));
    }

    chartWidget->setChartData(list, listTime, header);
    chartWidget->show();
}

/**
 * @brief MainWindow::showRowContextMenu
 * @param pos
 * @date  2023/10/12
 */
void MainWindow::showRowContextMenu(const QPoint &pos)
{
    QHeaderView* header = tableView->verticalHeader();

    // 创建右键菜单
    QMenu contextMenu;
    QAction *actDeleteRow = new QAction("删除行", &contextMenu);

    auto deleteTableRow = [=](){
        int row = header->logicalIndexAt(pos);
        tableModel->removeRow(row);
        tableView->update();
    };

    contextMenu.addAction(actDeleteRow);
    connect(actDeleteRow, &QAction::triggered, this, deleteTableRow);

    contextMenu.exec(header->mapToGlobal(pos));
}

/**
 * @brief TableView的列标题右键菜单
 * @param pos
 */
void MainWindow::showColContextMenu(const QPoint &pos)
{
    QHeaderView* header  = tableView->horizontalHeader();
    QString      colName = tableModel->headerData(header->logicalIndexAt(pos), Qt::Horizontal).toString();

    // 创建右键菜单
    QMenu    contextMenu;
    QAction *actCreateChart = new QAction("创建分析图表", &contextMenu);
    contextMenu.addAction(actCreateChart);

    auto createChart = [=](){
        int col = header->logicalIndexAt(pos);

        // 获取选中列的数据
        QList<double> listData;
        QStringList   listTime;
        for (int row = 0; row < tableModel->rowCount(); ++row) {
            QStandardItem * item = tableModel->item(row, col);
            QStandardItem * time = tableModel->item(row, tableModel->columnCount() - 1);
            if (item) {
                listData << item->text().toDouble();
                listTime << time->text();
            }
        }

        slotCreateChart(listData, listTime, colName);
    };

    connect(actCreateChart, &QAction::triggered, this, createChart);
    contextMenu.exec(header->mapToGlobal(pos)); // 在鼠标位置显示右键菜单
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：导入参考光谱
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/14
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotImportRefeSpectrum()
{
    QString xlsxName = QFileDialog::getOpenFileName(this, "打开xlsx文件", preferencesPageGeneral->defaultStoragePath, "Xlsx(*.xlsx)"
                                                    , nullptr, QFileDialog::DontUseNativeDialog);
    if (xlsxName.isEmpty()) {
        return;
    }

    QXlsx::Document xlsx(xlsxName);
    if (!xlsx.load()){
        QMessageBox::critical(this, StrMainWindowsTitle, "无法加载xlsx文件");
        return;
    }

    // 获取工作表
    QXlsx::Worksheet *sheet = xlsx.currentWorksheet();
    if (!sheet){
        QMessageBox::critical(this, StrMainWindowsTitle, "无法获取工作表");
        return;
    }

    // 获取行列
    const quint32 Rows = sheet->dimension().rowCount();
    const quint32 Cols = sheet->dimension().columnCount();
    if (Rows  < 2048 || Rows < 1 || Cols < 1){
        QMessageBox::warning(this, StrMainWindowsTitle, "Xlsx不合规范");
        return;
    }

    QVector<double> m_Array_ReferCurveData;
    m_Array_ReferCurveData.reserve(2048);

    for (quint32 i = 1; i <= Rows; i++){
        double sum = 0.0;
        for (quint32 j = 1; j <= Cols; j++){
            QXlsx::Cell *cell = sheet->cellAt(i, j);
            if (cell){
                sum += cell->value().toDouble();
            }
        }
        if (Cols != 0){
            m_Array_ReferCurveData.append(sum / Cols);
        }
    }

    // 发送参考光谱数据到分辨率计算页
    emit signalRefrenceSpectrumImported(&m_Array_ReferCurveData);

    static int colorIndex = 0;
    // 定义预定义颜色数组
    const QColor predefinedColors[5] = {
        Qt::magenta,
        QColor(65,  205, 82),
        QColor(205, 129, 0),
        QColor(119, 25,  170),
        Qt::red
    };

    QColor  curveColor = predefinedColors[colorIndex % 5];
    QString curveName  = "refer " + QString::number(listQwtPlotReferCurve.size() + 1);

    QwtPlotCurve *curve = chart->appendReferenceCurve(m_Array_ReferCurveData, curveName, curveColor);
    listQwtPlotReferCurve.append(curve);
    chart->legend()->show();
    ++colorIndex;
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：保存调试信息为txt文件
 * 函数参数：null
 * 链接对象：
 * 修改时间：2023/6/10
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotExportDebugInfomation()
{
    if (plainTextEdit_DebugInformation->toPlainText().isEmpty()) {
        QMessageBox::information(this, StrMainWindowsTitle, "没有调试数据可以保存！");
        return;
    }

    QString txtName = constructFileName("Debug", ".txt", true);
    if (exportDataToTxt(plainTextEdit_DebugInformation->toPlainText(), txtName))
        QMessageBox::information(this, StrMainWindowsTitle, "导出调试数据成功！");
    else
        QMessageBox::information(this, StrMainWindowsTitle, "导出调试数据失败！");  
}

void MainWindow::slotImportBackSpectrum()
{
    if (m_Flag_Import_Dark == false) {
        QMessageBox::information(this, StrMainWindowsTitle, "还未采集到暗电流，不能导入背景光谱！", QMessageBox::Information);
        return;
    }

    QString xlsxPath = QFileDialog::getOpenFileName(this, "打开xlsx文件", preferencesPageGeneral->defaultStoragePath, "Xlsx(*.xlsx)"
                                                    /*, nullptr, QFileDialog::DontUseNativeDialog*/);
    if (!QFile::exists(xlsxPath)){
        QMessageBox::critical(this, StrMainWindowsTitle, "xlsx文件不存在！");
        return;
    }

    QXlsx::Document xlsx(xlsxPath);
    if (!xlsx.load()){
        QMessageBox::critical(this, StrMainWindowsTitle, "无法加载暗光谱xlsx文件！");
        return;
    }

    // 获取工作表
    QXlsx::Worksheet *sheet = xlsx.currentWorksheet();
    if (!sheet) {
        QMessageBox::critical(this, StrMainWindowsTitle, "无法获取工作表！");
        return;
    }

    // 获取行列
    const quint32 Rows = sheet->dimension().rowCount();
    const quint32 Cols = sheet->dimension().columnCount();
    const int expectedRowCount = 2048;

    if (Rows  < expectedRowCount || Cols < 1) {
        QMessageBox::warning(this, StrMainWindowsTitle, "Xlsx不合规范");
        return;
    }

    // 将excel数据导入二维数组
    QVector<QVector<double>> m_Array_AllBack;
    QVector<double> m_Array_Back;
    m_Array_Back.reserve(2048);

    for (quint32 i = 1; i <= Cols; i++) {
        for (quint32 j = 1; j <= Rows; j++) {
            QXlsx::Cell *cell = sheet->cellAt(j, i);
            if (cell) {
                m_Array_Back.append(cell->value().toDouble());
            }
        }
        m_Array_AllBack.append(m_Array_Back);
    }

    // 找到并删除暗电流
    for (int i = m_Array_AllBack.size() - 1; i >= 0; i--) {
        if (checkDataFluctuation(m_Array_AllBack[i])) {
            m_Array_AllBack.removeAt(i);
        }
    }

    const int rows = m_Array_AllBack.size();
    const int cols = 2048;

    // 计算平均值
    for (int i = 0; i < cols; i++) {
        double sum = 0;
        for (int j = 0; j < rows; j++) {
            sum += m_Array_AllBack[j][i];
        }
        if (rows != 0) {
            m_Array_Back_Excel[i] = sum / rows;
            m_Array_Back_After[i] = m_Array_Back_Excel[i] - m_Array_Dark_Excel[i];
        }
    }

    m_Flag_Import_Back = true;
}

/**
 * @brief MainWindow::exportDataToTxt：将QString数据导出到txt文件
 * @param txtData：文本数据
 * @param txtName：要导出的文件名，注意这是完整路径
 * @return 成功返回true，否则返回false
 */
bool MainWindow::exportDataToTxt(const QString &txtData, const QString &txtName)
{
    // 检查文本数据和文件名是否为空
    if (txtData.isEmpty() || txtName.isEmpty())
        return false;

    // 创建一个 QFile 对象，打开文件以写入文本
    QFile file(txtName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // 文件无法打开，导出失败
        qDebug() << "Failed of open file：" << txtName;
        return false;
    }

    // 创建一个 QTextStream 对象，用于写入文本数据
    QTextStream out(&file);
    out << txtData; // 将数据写入文件

    // 关闭文件
    file.close();

    // 导出成功
    return true;
}

template<typename T>
bool MainWindow::exportDataToXlsx(const QList<QList<T> > &xlsxData, const QString &xlsxName)
{
    if (xlsxData.isEmpty() || xlsxName.isEmpty()) {
        qDebug() << __LINE__ << __FILE__ << __func__ << "Invalid input parameters";
        return false;
    }

    QXlsx::Document xlsx;
    for (int row = 0; row < xlsxData.size(); ++row) {
        for (int col = 0; col < xlsxData[row].size(); ++col) {
            xlsx.write(col + 1, row + 1, QVariant(xlsxData[row][col]));
        }
    }

    return xlsx.saveAs(xlsxName);
}

/**
 * @brief MainWindow::exportDataToXlsx: 用于保存二维数组数据到xlsx文件的函数，这是一个Tool函数和模板函数
 * @param xlsxData: 要保存到xlsx文件的数据，这是一个二维数组
 * @param xlsxName: 要设置的xlsx文件的文件名，后缀一般使用.xlsx，在这个函数不是必须的
 * @return 保存成功返回true，反之，返回false
 */
template<typename T>
bool MainWindow::exportDataToXlsx(const QVector<QVector<T>> &xlsxData, const QString &xlsxName)
{
    if (xlsxData.isEmpty() || xlsxName.isEmpty()) {
        qDebug() << __LINE__ << __FILE__ << __func__ << "Invalid input parameters";
        return false;
    }

    QXlsx::Document xlsx;
    for (int row = 0; row < xlsxData.size(); ++row) {
        for (int col = 0; col < xlsxData[row].size(); ++col) {
            xlsx.write(col + 1, row + 1, QVariant(xlsxData[row][col]));
        }
    }

    return xlsx.saveAs(xlsxName);
}

/**
 * @brief MainWindow::checkDataFluctuation: 检查光谱数据是不是暗电流
 * @param data
 * @return 如果是暗电流，返回true，否则返回false
 * @date: 2023/8/22
 */
bool MainWindow::checkDataFluctuation(const QVector<double> &data)
{
    if (data.size() < 100) {
        return false;   // 数据点数量不足100个
    }

    for (int i = 0; i < 100; ++i) {
        int diff = std::abs(data[i] - data[i + 1]);
        if (diff > 100) {       
            return false;   // 波动范围超过100
        }
    }

    // 前100个数据点的波动范围都在100以内
    return true;
}

QByteArray MainWindow::Unescaped(const QByteArray &frame, const int length)
{
    QByteArray array;
    array.reserve(length);

    for (int i = 0; i < length; ++i) {
        if (frame[i] == char(0xF5)) {
            if (frame[i+1] == char(0x00)) {
                array.append(0xF0);
                ++i;
                continue;
            } else if (frame[i+1] == char(0x05)) {
                array.append(0xF5);
                ++i;
                continue;
            } else if (frame[i+1] == char(0x0F)) {
                array.append(0xFF);
                ++i;
                continue;
            }else {
                array.append(frame[i]);
            }
        } else {
            array.append(frame[i]);
        }
    }
    return array;
}

/**
 * @brief 开始采集按钮链接的槽
 * @note
 */
void MainWindow::slotStartOrStopCollecting()
{
    // 判断是否有可以使用的串口
    if (cboxPortName->count() == 0) {
        QMessageBox::information(this, StrMainWindowsTitle, "没有可以使用的串口！");
        return;
    }

    // 判断是开始还是停止采集
    if (btnStartCollect->text() == "开始采集") {
        /* 获取串口 */
        QString portInfo = cboxPortName->currentText();
        QString portName = portInfo.left(portInfo.indexOf(' '));

        /* 配置串口 */
        serialPort->setPortName(portName);
        serialPort->setBaudRate(preferencesPageGeneral->baudRate);
        serialPort->setDataBits(preferencesPageGeneral->dataBits);
        serialPort->setStopBits(preferencesPageGeneral->stopBits);
        serialPort->setParity(preferencesPageGeneral->parityBits);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        if (serialPort->open(QIODevice::ReadWrite)) {

            /* 发送采集指令 */
            int  bytes  = serialPort->write("open");
            bool result = serialPort->waitForBytesWritten();

            if (!result || bytes == -1 || bytes != 4) {
                QMessageBox::information(this, StrMainWindowsTitle, "开始采集失败！");
                return;
            }

            switch (cboxPortMode->currentIndex()) {
            case 0:
                type = Type::collType_ModuleCollecting;   // 采集模块

                slotResetQwtPlotAxis();
                actionModuleDebug->setEnabled(true);
                actionSpectrumDebug->setEnabled(false);
                break;
            case 1:
                type = Type::collType_SpectrumCollecting;

                slotResetQwtPlotAxis();
                actionModuleDebug->setEnabled(false);
                actionSpectrumDebug->setEnabled(true);
                break;
            case 2:
                if (!m_Flag_Import_Back) {
                    serialPort->close();
                    QMessageBox::information(this, StrMainWindowsTitle, "请先导入背景光谱");
                    return;
                }

                if (!m_Flag_Import_Dark) {
                    serialPort->close();
                    QMessageBox::information(this, StrMainWindowsTitle, "请先导入暗光谱");
                    return;
                }
                type = Type::collType_ModuleAbsorb;

                slotResetQwtPlotAxis();
                actionModuleDebug->setEnabled(true);
                actionSpectrumDebug->setEnabled(false);
                break;
            case 3:     // 采集GFC模块
                type = Type::collType_GFCModuleCollecting;

                slotResetQwtPlotAxis();
                actionModuleDebug->setEnabled(false);
                actionSpectrumDebug->setEnabled(false);
                break;
            }

            // 连接信号槽函数
            serialPort->clear();    // 清空串口接收缓冲区
            QObject::connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::slotReadAllSerialPortData);

            // 打开成功：更新UI
            cboxPortMode->setDisabled(true);
            cboxPortName->setDisabled(true);
            btnStartCollect->setText("停止采集");
            labelSerialOpen->setText("连接到串口：" + portName);
        } else {
            // 打开失败：弹窗报错
            QMessageBox::critical(this, StrMainWindowsTitle, QString("打开串口") + portName + QString("失败：") + serialPort->errorString());
            return;
        }
    } else {
        // 采集标识复位
        type = Type::collType_Default;

        // 断开信号槽函数
        QObject::disconnect(serialPort, &QSerialPort::readyRead, this, &MainWindow::slotReadAllSerialPortData);
        serialPort->close();

        cboxPortMode->setEnabled(true);
        cboxPortName->setEnabled(true);
        btnStartCollect->setText("开始采集");

        // 清空调试信息页
        emit signalClearInformationPage();
        actionModuleDebug->setEnabled(true);
        actionSpectrumDebug->setEnabled(true);

        // 关闭连续保存
        Timer.stop();
//        ui->pushButton_EnableConSave->setText("开始");

        // 底栏复位
        labelSerialOpen->setText(StrSerialPortNoOpen);
        labelSpectromNumber->setText(StrSpectrumNumber + StrNull);
        labelSoftwareNumber->setText(StrSoftwareNumber + StrNull);
        labelSpectromTemper->setText(StrSpectrumTemper + StrNull);
        labelChamberPressure->setText(StrChamberPressure + StrNull);
        labelChamberTemper->setText(StrChamberTemper + StrNull);
        labelSpectrumIntervals -> setText(StrGetSpectrumTime + StrNull);
    }
}

/*************************************************
 * 函数类别：自定义槽函数
 * 函数功能：
 * 函数参数：null
 * 返回对象：
 * 链接对象：
 * 修改时间：2023/7/20
 * 联系邮箱：2282669851@qq.com
 *************************************************/
void MainWindow::slotReadAllSerialPortData()
{
#ifdef PERFORMANCE_TESTING
    auto start = std::chrono::high_resolution_clock::now();
#endif
    buffer.append(serialPort->readAll());

    switch (type) {
    case Type::collType_ModuleAbsorb:
    case Type::collType_ModuleCollecting:
        extractAndProcessSerialFrames(buffer, MFrame_Head, MFrame_Tail, FRAME_HEAD_LENGTH, FRAME_TAIL_LENGTH);
        break;

    case Type::collType_SpectrumCollecting:
        extractAndProcessSerialFrames(buffer, SFrame_Head, SFrame_Tail, SELFFRAME_HEAD_LENGTH, SELFFRAME_TAIL_LENGTH);
        break;

    case Type::collType_GFCModuleCollecting:
        break;

    case Type::collType_Default:
    default:
        break;
    }

#if 1

#else
    // 2023/09/19
    if (flagModuleCollecting && buffer.contains(MFrame_Tail)) {
        int headIndex = buffer.indexOf(MFrame_Head);
        int tailIndex = buffer.indexOf(MFrame_Tail, headIndex);

        if (tailIndex != -1) {
            QByteArray frame = buffer.mid(headIndex + FRAME_HEAD_LENGTH, tailIndex - headIndex - FRAME_HEAD_LENGTH);
            frame = Unescaped(frame, frame.length());
            frame.append(MFrame_Tail);
            frame.prepend(MFrame_Head);
            processModuleFrameData(frame);
            buffer.remove(0, tailIndex - headIndex + FRAME_TAIL_LENGTH);
        }
    }

    if (flagSpectrumCollecting && buffer.contains(SFrame_Tail)) {
        int headIndex = buffer.indexOf(SFrame_Head);
        int tailIndex = buffer.indexOf(SFrame_Tail, headIndex);

        if (tailIndex != -1) {
            processSpectrumFrameData(buffer.mid(headIndex, tailIndex - headIndex + SELFFRAME_TAIL_LENGTH));
            buffer.remove(0, tailIndex - headIndex + SELFFRAME_TAIL_LENGTH);
        }
    }
#endif

#ifdef PERFORMANCE_TESTING
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    qDebug() << "执行时间：" << duration.count() << " 毫秒";
#endif
}

/**
 * @brief MainWindow::extractAndProcessSerialFrames
 * @note  根据数据帧头和数据帧尾从串口中截取出一阵数据并进行处理
 * @param buffer
 * @param head
 * @param tail
 * @param headLength
 * @param tailLength
 */
void MainWindow::extractAndProcessSerialFrames(QByteArray &buffer, const QByteArray &head, const QByteArray &tail, const int headLength, const int tailLength)
{
    // 检查缓冲区中是否存在完整帧
    while (buffer.length() >= (headLength + tailLength)) {
        // 查找数据帧头
        int headIndex = buffer.indexOf(head);
        if (headIndex == -1) {
            buffer.clear();
            break;          // 如果没有找到数据帧头，则清空缓冲区并退出当前循环
        }
        if (headIndex > 0) {
            buffer.remove(0, headIndex);
            headIndex = 0;  // 如果帧头不在第一个位置，则丢弃前面的数据
        }
        // 查找数据帧尾
        int tailIndex = buffer.indexOf(tail);
        if (tailIndex == -1) {
            break;          // 如果没有找到数据帧尾，则退出当前循环
        }
        // 提取完整数据帧（去除帧头和帧尾）
        QByteArray frame = buffer.mid(headIndex + headLength, tailIndex - headIndex - headLength);
        buffer.remove(0, tailIndex + tailLength);   // 删除已经处理的数据，并继续查找下一个帧头

        // 取消使用信号槽改为直接调用，效率更高
        switch (type) {
        case Type::collType_ModuleAbsorb:
        case Type::collType_ModuleCollecting:
            frame = Unescaped(frame, frame.length());
            frame.append(tail);
            frame.prepend(head);
            processModuleFrameData(frame);
            break;

        case Type::collType_SpectrumCollecting:
            frame.append(tail);
            frame.prepend(head);
            processSpectrumFrameData(frame);
            break;

        case Type::collType_GFCModuleCollecting:
            break;
        default:
            break;
        }
    }
}

void MainWindow::processModuleFrameData(const QByteArray &frame)
{
    if (frame.length() != 5120) {
        qDebug() << "数据长度不正确: " << frame.length();
        return;
    }

    // CRC16校验
    quint16 crc_V = CRC16(frame, 0, 5116);
    quint8  crc_l = crc_V & 0xFF;
    quint8  crc_h = (crc_V >> 8) & 0xFF;

    if ((crc_h != static_cast<quint8>(frame.at(5117))) || (crc_l != static_cast<quint8>(frame.at(5118)))) {
        qDebug() << "CRC校验未通过！";
        return;
    }

    // 计算间隔时间
    if (!m_timer.isValid()) {
        m_timer.start();
    } else {
        qint64 elapsed = m_timer.elapsed();
        QString intervalText = QString("%1 %2").arg(StrGetSpectrumTime).arg(static_cast<double>(elapsed) / 1000.0);
        labelSpectrumIntervals->setText(intervalText);
        m_timer.restart();
    }

    // 指针
    quint8 *_ptr = nullptr;
    QString moduleNumber;    // 模块型号
    QString softwareVersion; // 软件版本

    for (auto i = 0; i < 30; i++) {
        moduleNumber += frame.at(3 + i);
    }
    for (auto i = 0; i < 14; i++) {
        softwareVersion += frame.at(33 + i);
    }

    if (moduleDebugPage != nullptr) {
        moduleDebugPage->Interval      = static_cast<quint8>(frame.at(47));
        moduleDebugPage->LigthingTimes = static_cast<quint8>(frame.at(48));
        moduleDebugPage->AddTimes      = static_cast<quint8>(frame.at(49));
        moduleDebugPage->XenonEnergy   = static_cast<quint8>(frame.at(50));
        moduleDebugPage->SpactrumType  = static_cast<quint8>(frame.at(63));
    }

    // 零点校准完成状态
    quint8 zeroCaliStatus = static_cast<quint8>(frame.at(4160));

    quint16 modState;           // 模块状态
    float pressure = 0.0f;      // 气室压力
    float temp_spc = 0.0f;      // 光谱温度
    float temp_gas = 0.0f;      // 气室温度
    int wavelengthShift = 0;    // 波长漂移

    _ptr = reinterpret_cast<quint8*>(&modState);
    _ptr[1] = static_cast<quint8>(frame[4161]);
    _ptr[0] = static_cast<quint8>(frame[4162]);

    _ptr = reinterpret_cast<quint8*>(&pressure);
    _ptr[3] = static_cast<quint8>(frame[4163]);
    _ptr[2] = static_cast<quint8>(frame[4164]);
    _ptr[1] = static_cast<quint8>(frame[4165]);
    _ptr[0] = static_cast<quint8>(frame[4166]);

    _ptr = reinterpret_cast<quint8*>(&temp_spc);
    _ptr[3] = static_cast<quint8>(frame[4167]);
    _ptr[2] = static_cast<quint8>(frame[4168]);
    _ptr[1] = static_cast<quint8>(frame[4169]);
    _ptr[0] = static_cast<quint8>(frame[4170]);

    _ptr = reinterpret_cast<quint8*>(&temp_gas);
    _ptr[3] = static_cast<quint8>(frame[4171]);
    _ptr[2] = static_cast<quint8>(frame[4172]);
    _ptr[1] = static_cast<quint8>(frame[4173]);
    _ptr[0] = static_cast<quint8>(frame[4174]);

    _ptr = reinterpret_cast<quint8*>(&wavelengthShift);
    _ptr[3] = static_cast<quint8>(frame[4175]);
    _ptr[2] = static_cast<quint8>(frame[4176]);
    _ptr[1] = static_cast<quint8>(frame[4177]);
    _ptr[0] = static_cast<quint8>(frame[4178]);

    // 传输气体浓度
    float arr_conc_mg[8];
    float arr_conc_ppm[8];

    for (auto i = 0; i < 8; i++) {
        _ptr = reinterpret_cast<quint8*>(&arr_conc_mg[i]);
        _ptr[3] = static_cast<quint8>(frame[4179+4*i+0]);
        _ptr[2] = static_cast<quint8>(frame[4179+4*i+1]);
        _ptr[1] = static_cast<quint8>(frame[4179+4*i+2]);
        _ptr[0] = static_cast<quint8>(frame[4179+4*i+3]);

        _ptr = reinterpret_cast<quint8*>(&arr_conc_ppm[i]);
        _ptr[3] = static_cast<quint8>(frame[4211+4*i+0]);
        _ptr[2] = static_cast<quint8>(frame[4211+4*i+1]);
        _ptr[1] = static_cast<quint8>(frame[4211+4*i+2]);
        _ptr[0] = static_cast<quint8>(frame[4211+4*i+3]);
    }

    QVector<double> m_Array_CurrtCurveData;
    m_Array_CurrtCurveData.reserve(2048);

    // 解析光谱数据
    for (auto i = 0; i < 2048; i++) {
        m_Array_CurrtCurveData.append(
            static_cast<double>(
            static_cast<quint8>(frame[64 + i * 2]) * 0x100 +
            static_cast<quint8>(frame[64 + i * 2 + 1]))
        );
    }

    // 添加到二维光谱数据数组
//    m_Array_AllCurveData.append(m_Array_CurrtCurveData);

    // 如果开启了实时计算，那么发送信号，发送当前光谱数据
    if (resolutionCalculationPage && resolutionCalculationPage->isCalculating) {
        emit signalSendCurrentSerialPortData(m_Array_CurrtCurveData);
    }
    if (driftCalculationPage != nullptr && driftCalculationPage->isCalculating) {
        emit signalSendCurrentSerialPortData(m_Array_CurrtCurveData);
    }

    // 查找暗电流
    if (checkDataFluctuation(m_Array_CurrtCurveData)) {
        if (!m_Flag_Import_Dark) {
            m_Flag_Import_Dark = true;
        }

        m_Array_Dark_Excel = m_Array_CurrtCurveData;

        // 处理背景光谱
        for (int i = 0; i < 2048; i++) {
            m_Array_Back_After[i] = m_Array_Back_Excel[i] - m_Array_Dark_Excel[i];
        }
    }

    // 采集吸光度
    if (type == Type::collType_ModuleAbsorb) {
        QVector<double> curveData;
        curveData.reserve(2048);

        for (int i = 0; i < 2048; i++) {
            double denominator = m_Array_CurrtCurveData[i] - m_Array_Dark_Excel[i];
            if (denominator > 0) {
                curveData.append(log(m_Array_Back_After[i] / denominator));
            } else {
                // 处理负数情况，例如给CurveData.append赋一个特殊的值
                curveData.append(-1.0);
            }
        }

        QColor randomColor = QColor::fromRgb(QRandomGenerator::global()->bounded(256),
                                             QRandomGenerator::global()->bounded(256),
                                             QRandomGenerator::global()->bounded(256));
        Q_UNUSED(randomColor)

        QwtPlotCurve* curve = chart->appendDefaultCurve(curveData);
        listQwtPlotCurve.append(curve);
    } else {

        QwtPlotCurve* curve = chart->appendDefaultCurve(m_Array_CurrtCurveData);
        listQwtPlotCurve.append(curve);
    }

    // Max Curve Setting
    int curveCount = removeExcessCurves(chart, MaxCurveNum);
    labelSpectrumCount ->setText(StrSpectrumCount  + QString::number(curveCount));

// 调试信息
#if 1
    const int DebugIndex_S = 4243;
    const int DebugIndex_E = 5114;
    const int DebugInfoLength = DebugIndex_E - DebugIndex_S + 1;

    QByteArray arrDebugInfo = frame.mid(4243, DebugInfoLength);
    QString strDebugInfo = QString::fromLatin1(arrDebugInfo.data(), arrDebugInfo.size());
#endif

    /* 调试数据整理 */
    QString strModuleState = "0X" + QString::number(modState, 16);          // 模块状态
    QString strAirChamberPressure     = QString::number(pressure, 'f', 3);  // 气室压力
    QString strAirChamberTemperature  = QString::number(temp_gas, 'f', 3);  // 气室温度
    QString strSpectrometerTemerature = QString::number(temp_spc, 'f', 3);  // 光谱仪温度
    QString strWavelengthShift        = QString::number(wavelengthShift);   // 波长漂移量

    // 气体mg
    QString str_mg_SO2 = QString::number(arr_conc_mg[0], 'f', 3);
    QString str_mg_NO2 = QString::number(arr_conc_mg[1], 'f', 3);
    QString str_mg_NO  = QString::number(arr_conc_mg[2], 'f', 3);
    QString str_mg_H2S = QString::number(arr_conc_mg[3], 'f', 3);
    QString str_mg_NH3 = QString::number(arr_conc_mg[4], 'f', 3);

    // 气体ppm
    QString str_ppm_SO2 = QString::number(arr_conc_ppm[0], 'f', 3);
    QString str_ppm_NO2 = QString::number(arr_conc_ppm[1], 'f', 3);
    QString str_ppm_NO  = QString::number(arr_conc_ppm[2], 'f', 3);
    QString str_ppm_H2S = QString::number(arr_conc_ppm[3], 'f', 3);
    QString str_ppm_NH3 = QString::number(arr_conc_ppm[4], 'f', 3);

    QStringList rowDataList;
    rowDataList << strModuleState
                << strAirChamberPressure
                << strAirChamberTemperature
                << strSpectrometerTemerature
                << strWavelengthShift
                << str_mg_SO2
                << str_mg_NO2
                << str_mg_NO
                << str_mg_H2S
                << str_mg_NH3
                << str_ppm_SO2
                << str_ppm_NO2
                << str_ppm_NO
                << str_ppm_H2S
                << str_ppm_NH3
                << QDateTime::currentDateTime().toString("MM-dd-hh:mm:ss");

    /* 将调试数据导入到调试数据表格 */
    QList<QStandardItem*> itemList; // 行数据容器

    for (const auto& rowData : qAsConst(rowDataList)) {
        itemList.append(new QStandardItem(rowData));
    }

    tableModel->appendRow(itemList);

    // 如果滑块在底部，将滚动到最新行
    QScrollBar* verticalScrollBar = tableView->verticalScrollBar();
    if (verticalScrollBar->value() == verticalScrollBar->maximum()) {
        tableView->scrollToBottom();
    }

//    QStandardItem *rowHeaderItem = new QStandardItem(QString::number(tableModel->rowCount() + 1));
//    tableModel->setVerticalHeaderItem(tableModel->rowCount() - 1, rowHeaderItem);

//    QModelIndex curIndex = tableModel->index(tableModel->rowCount() - 1, 0);
//    tableView->setCurrentIndex(curIndex);

    QString outputDebug;    // 输出的调试数据

    if (cBoxAddTimestamp->checkState()) {    // 是否加时间戳
        QString outputTime = QDateTime::currentDateTime().toString("[yyyy-MM-dd-hh:mm:ss] ");
        outputDebug.push_front(outputTime);
    }

    outputDebug += strModuleState + ' ';
    outputDebug += strAirChamberPressure + ' ';
    outputDebug += strAirChamberTemperature + ' ';
    outputDebug += strSpectrometerTemerature + ' ';
    outputDebug += strWavelengthShift + ' ';
    outputDebug += str_mg_NO2  + ' ';
    outputDebug += str_mg_SO2  + ' ';
    outputDebug += str_mg_NO   + ' ';
    outputDebug += str_mg_NH3  + ' ';
    outputDebug += str_mg_H2S  + ' ';
    outputDebug += str_ppm_NO2 + ' ';
    outputDebug += str_ppm_SO2 + ' ';
    outputDebug += str_ppm_NO  + ' ';
    outputDebug += str_ppm_NH3 + ' ';
    outputDebug += str_ppm_H2S + ' ';

// 删除调试信息 * 号左边的所有数据
#if 0
    int starIndex = strDebugInfo.indexOf('*');
    QString strDebugInfoAfterstar = strDebugInfo.right(starIndex);
    outputDebug += strDebugInfoAfterstar;
#endif

#if 1
//            strDebugInfo.remove(QRegExp("\\s+"));         // 删除所有空白字符
//            strDebugInfo = strDebugInfo.trimmed();        // 删除开头和结尾的空白字符
    strDebugInfo.remove(QRegExp("[\\t\\n\\r]"));    // 删除除了空格之外的空白字符
    outputDebug += strDebugInfo;
#endif
    // 添加调试数据到调试信息窗口
    plainTextEdit_DebugInformation->appendPlainText(outputDebug);

    // 接收调试数据到文件
    if (cboxReceiveToFile->checkState())  {
        QTextStream outStreamDebug(debugFile);
        outStreamDebug << outputDebug << "\r\n";
    }

    // 更新底栏
    labelSpectromNumber->setText(StrSpectrumNumber +   moduleNumber);
    labelSoftwareNumber->setText(StrSoftwareNumber +   softwareVersion);
    labelSpectromTemper->setText(StrSpectrumTemper +   strSpectrometerTemerature);
    labelChamberPressure->setText(StrChamberPressure + strAirChamberPressure);
    labelChamberTemper->setText(StrChamberTemper + strAirChamberTemperature);

    // 当打开调试页时
    if (moduleDebugPage != nullptr) {
        moduleDebugPage->ui->lineEdit_ModuleNumb->setText(moduleNumber);
        moduleDebugPage->ui->lineEdit_SoftwareVersion->setText(softwareVersion);

        // 只设置一次的Item
        if (moduleDebugPage->resetFlag) {
            moduleDebugPage->resetFlag = false;
            moduleDebugPage->ui->lineEdit_Interval->setText(QString::number(moduleDebugPage->Interval));
            moduleDebugPage->ui->lineEdit_AddTimes->setText(QString::number(moduleDebugPage->AddTimes));
            moduleDebugPage->ui->lineEdit_XenonEnergy->setText(QString::number(moduleDebugPage->XenonEnergy));
            moduleDebugPage->ui->lineEdit_LigthingTimes->setText(QString::number(moduleDebugPage->LigthingTimes));
        }

        if (zeroCaliStatus == 0) {
            moduleDebugPage->ui->label_ZeroCaliState->setText("未校准");
            moduleDebugPage->ui->pushButton_ZeroCaliState->setEnabled(true);
        } else if (zeroCaliStatus == 1) {
            moduleDebugPage->ui->label_ZeroCaliState->setText("已校准");
            moduleDebugPage->ui->pushButton_ZeroCaliState->setDisabled(true);
        }

        moduleDebugPage->ui->lineEdit_ModuleState->setText(strModuleState);
        if (modState == 0x00){
            moduleDebugPage->ui->label_ModuleState->setText("预热中");
        }
        if (modState == 0x200){
            moduleDebugPage->ui->label_ModuleState->setText("预热完成");
        }

        moduleDebugPage->ui->lineEdit_ChamberPressure->setText(strAirChamberPressure);
        moduleDebugPage->ui->lineEdit_ChamberTemperature->setText(strAirChamberTemperature);
        moduleDebugPage->ui->lineEdit_SpectrometerTempreature->setText(strSpectrometerTemerature);
        moduleDebugPage->ui->lineEdit_WaveLengthShift->setText(strWavelengthShift);

        moduleDebugPage->ui->lineEdit_mg_SO2->setText(str_mg_SO2);
        moduleDebugPage->ui->lineEdit_mg_NO2->setText(str_mg_NO2);
        moduleDebugPage->ui->lineEdit_mg_NO ->setText(str_mg_NO);
        moduleDebugPage->ui->lineEdit_mg_H2S->setText(str_mg_H2S);
        moduleDebugPage->ui->lineEdit_mg_NH3->setText(str_mg_NH3);

        moduleDebugPage->ui->lineEdit_ppm_SO2->setText(str_ppm_SO2);
        moduleDebugPage->ui->lineEdit_ppm_NO2->setText(str_ppm_NO2);
        moduleDebugPage->ui->lineEdit_ppm_NO ->setText(str_ppm_NO);
        moduleDebugPage->ui->lineEdit_ppm_H2S->setText(str_ppm_H2S);
        moduleDebugPage->ui->lineEdit_ppm_NH3->setText(str_ppm_NH3);
    }

    if (moduleDebugPage != nullptr && moduleDebugPage->settingFlag) {
        moduleDebugPage->settingFlag = false;
        moduleDebugPage->ui->pushButton_SetInterval->setEnabled(true);
        moduleDebugPage->ui->pushButton_SetAddTimes->setEnabled(true);
        moduleDebugPage->ui->pushButton_ZeroCaliState->setEnabled(true);
        moduleDebugPage->ui->pushButton_SetXenonEnergy->setEnabled(true);
        moduleDebugPage->ui->pushButton_SetLigthingTimes->setEnabled(true);

        if (quint8(frame[5116]) == quint8(0x01)) {
            moduleDebugPage->isSuccess = true;
        } else {
            moduleDebugPage->isSuccess = false;
        }

        switch (quint8(frame[5115])) {
        case 0x01:
            if (moduleDebugPage->isSuccess)
                QMessageBox::information(this, StrMainWindowsTitle, "设置采集间隔成功！");
            else
                QMessageBox::information(this, StrMainWindowsTitle, "设置采集间隔失败！");
            break;
        case 0x02:
            if (moduleDebugPage->isSuccess)
                QMessageBox::information(this, StrMainWindowsTitle, "设置亮灯次数成功！");
            else
                QMessageBox::information(this, StrMainWindowsTitle, "设置亮灯次数失败！");
            break;
        case 0x03:
            if (moduleDebugPage->isSuccess)
                QMessageBox::information(this, StrMainWindowsTitle, "设置叠加次数成功！");
            else
                QMessageBox::information(this, StrMainWindowsTitle, "设置叠加次数失败！");
            break;
        case 0x04:
            if (moduleDebugPage->isSuccess)
                QMessageBox::information(this, StrMainWindowsTitle, "设置氙灯调电压值成功！");
            else
                QMessageBox::information(this, StrMainWindowsTitle, "设置氙灯调电压值失败！");
            break;
        case 0x05:
            if (moduleDebugPage->isSuccess)
                QMessageBox::information(this, StrMainWindowsTitle, "发送零点校准指令成功！");
            else
                QMessageBox::information(this, StrMainWindowsTitle, "发送零点校准指令失败！");
            break;
        default:
            QMessageBox::information(this, StrMainWindowsTitle, "未知指令！");
            break;
        }
    }
}


void MainWindow::processSpectrumFrameData(const QByteArray &frame)
{
    if (frame.length() != 4154) {
        qDebug() << "数据长度不正确: " << frame.length();
        return;
    }

    quint16 crc_V  = CRC16(frame, 0, 4147);
    quint8  crc_l  = crc_V & 0xFF;
    quint8  crc_h  = (crc_V >> 8) & 0xFF;

    if ((crc_h != quint8(frame[4148])) || (crc_l != quint8(frame[4149]))) {
        qDebug() << "CRC校验未通过！";
        return;
    }

    // 计算间隔时间
    if (!m_timer.isValid()) {
        m_timer.start();
    }else {
        qint64 elapsed = m_timer.elapsed();
        labelSpectrumIntervals->setText(StrGetSpectrumTime + QString::number(elapsed / 1000.0));
        m_timer.restart();
    }

    // 模块型号 char
    QByteArray moduleNumb;
    for (auto i = 0; i < 19; i++){
        moduleNumb.append(frame[6 + i]);
    }
    labelSpectromNumber->setText(StrSpectrumNumber + moduleNumb);

    // 软件版本 char
    QByteArray softwareVersion;
    for (auto i = 0; i < 14; i++){
        softwareVersion.append(frame[24 + i]);
    }
    labelSoftwareNumber->setText(StrSoftwareNumber + softwareVersion);

    float temp_spectrometer; // 光谱仪温度
    quint8 *temp_ptr = reinterpret_cast<quint8*>(&temp_spectrometer);
    temp_ptr[3] = static_cast<quint8>(frame[4142]);
    temp_ptr[2] = static_cast<quint8>(frame[4143]);
    temp_ptr[1] = static_cast<quint8>(frame[4144]);
    temp_ptr[0] = static_cast<quint8>(frame[4145]);
    labelSpectromTemper->setText(StrSpectrumTemper + QString::number(temp_spectrometer));

    if (spectromDebugPage != nullptr) {
        // 光谱仪序列号
        spectromDebugPage->SpectromNumber = moduleNumb;
        spectromDebugPage->ui->lineEdit_SpectromNumber->setText(moduleNumb);

        // 软件版本号
        spectromDebugPage->ui->lineEdit_SoftwareVersion->setText(softwareVersion);

        // 采集间隔
        spectromDebugPage->Interval  = static_cast<quint16>(static_cast<quint8>(frame[38])) << 8;
        spectromDebugPage->Interval |= static_cast<quint16>(static_cast<quint8>(frame[39]));

        // 曝光时间
        spectromDebugPage->ExposureTime  = static_cast<quint32>(static_cast<quint8>(frame[43])) << 24;
        spectromDebugPage->ExposureTime |= static_cast<quint32>(static_cast<quint8>(frame[42])) << 16;
        spectromDebugPage->ExposureTime |= static_cast<quint32>(static_cast<quint8>(frame[41])) << 8;
        spectromDebugPage->ExposureTime |= static_cast<quint32>(static_cast<quint8>(frame[40]));

        // 叠加次数
        spectromDebugPage->AddTimes = static_cast<quint8>(frame[44]);

        // 暗电流值
        spectromDebugPage->DarkCurrent = static_cast<quint8>(frame[45]);

        // 光谱仪温度
        spectromDebugPage->ui->lineEdit_SpectromTemp->setText(QString::number(temp_spectrometer, 'f', 3));

        if (spectromDebugPage->resetFlag) {
            spectromDebugPage->resetFlag = false;
            spectromDebugPage->ui->lineEdit_Interval->setText(QString::number(spectromDebugPage->Interval));
            spectromDebugPage->ui->lineEdit_AddTimes->setText(QString::number(spectromDebugPage->AddTimes));
            spectromDebugPage->ui->lineEdit_ExposureTime->setText(QString::number(spectromDebugPage->ExposureTime));
            spectromDebugPage->ui->lineEdit_DarkCurrentValue->setText(QString::number(spectromDebugPage->DarkCurrent));
        }
    }

    QVector<double> m_Array_CurrtCurveData;
    m_Array_CurrtCurveData.reserve(2048);

    // 光谱数据
    for (auto i = 0; i < 2048; i++) {
        m_Array_CurrtCurveData.append(
            static_cast<double>(
            static_cast<quint8>(frame[i*2+46]) * 0x100 +
            static_cast<quint8>(frame[i*2+1+46]))
        );
    }
//    m_Array_AllCurveData.append(m_Array_CurrtCurveData);

    // 如果开启了实时计算，那么发送信号，发送当前光谱数据
    if (resolutionCalculationPage && resolutionCalculationPage->isCalculating) {
        emit signalSendCurrentSerialPortData(m_Array_CurrtCurveData);
    }
    if (driftCalculationPage != nullptr && driftCalculationPage->isCalculating) {
        emit signalSendCurrentSerialPortData(m_Array_CurrtCurveData);
    }

    QwtPlotCurve* curve = chart->appendDefaultCurve(m_Array_CurrtCurveData);
    listQwtPlotCurve.append(curve);

    // Max Curve Setting
    int curveCount = removeExcessCurves(chart, MaxCurveNum);
    labelSpectrumCount ->setText(StrSpectrumCount  + QString::number(curveCount));

    if (spectromDebugPage != nullptr && spectromDebugPage->settingFlag) {
        spectromDebugPage->settingFlag = false;
        spectromDebugPage->ui->pushButton_Reset->setEnabled(true);
        spectromDebugPage->ui->pushButton_SetInterval->setEnabled(true);
        spectromDebugPage->ui->pushButton_SetAddTimes->setEnabled(true);
        spectromDebugPage->ui->pushButton_SetExposureTime->setEnabled(true);
        spectromDebugPage->ui->pushButton_SetDarkCurrentValue->setEnabled(true);
        spectromDebugPage->ui->pushButton_SetSerialNumber->setEnabled(true);

        if (static_cast<quint8>(frame[4147]) == 0x00) {
            spectromDebugPage->isSuccess = false;
        } else {
            spectromDebugPage->isSuccess = true;
        }

        switch (static_cast<quint8>(frame[4146])) {
        case 0x01:
            if (spectromDebugPage->isSuccess) {
                QMessageBox::information(this, "提示", "设置触发方式成功！");
            } else {
                QMessageBox::information(this, "提示", "设置触发方式失败！");
            }
            spectromDebugPage->isSuccess = false;
            break;
        case 0x02:
            if (spectromDebugPage->isSuccess) {
                if (spectromDebugPage->flagSetExp)
                    QMessageBox::information(this, "提示", "设置曝光时间成功！");
                if (spectromDebugPage->flagSetAdd)
                    QMessageBox::information(this, "提示", "设置叠加次数成功！");
                spectromDebugPage->flagSetExp = false;
                spectromDebugPage->flagSetAdd = false;
            }
            else {
                if (spectromDebugPage->flagSetExp)
                    QMessageBox::information(this, "提示", "设置曝光时间失败！");
                if (spectromDebugPage->flagSetAdd)
                    QMessageBox::information(this, "提示", "设置叠加次数失败！");
                spectromDebugPage->flagSetExp = false;
                spectromDebugPage->flagSetAdd = false;
            }
            spectromDebugPage->isSuccess = false;
            break;
        case 0x06:
            if (spectromDebugPage->isSuccess) {
                QMessageBox::information(this, "提示", "设置暗电流值成功！");
            } else {
                QMessageBox::information(this, "提示", "设置暗电流值失败！");
            }
            spectromDebugPage->isSuccess = false;
            break;
        case 0x08:
            if (spectromDebugPage->isSuccess) {
                QMessageBox::information(this, "提示", "设置采集间隔成功！");
            } else {
                QMessageBox::information(this, "提示", "设置采集间隔失败！");
            }
            spectromDebugPage->isSuccess = false;
            break;
        case 0x09:
            if (spectromDebugPage->isSuccess) {
                QMessageBox::information(this, "提示", "设置光谱仪序列号成功！");
            } else {
                QMessageBox::information(this, "提示", "设置光谱仪序列号失败！");
            }
            spectromDebugPage->isSuccess = false;
            break;
        case 0x10:
            if (spectromDebugPage->isSuccess) {
                QMessageBox::information(this, "提示", "恢复出厂设置成功！");
            } else {
                QMessageBox::information(this, "提示", "恢复出厂设置失败！");
            }
            spectromDebugPage->isSuccess = false;
            break;
        default:
            spectromDebugPage->isSuccess = false;
            QMessageBox::information(this, "提示", QString("设置失败!") + QString("错误：") + QString::number(frame[4146]));
            break;
        }
    }
}

/**
 * @brief   删除多余的曲线，用于设置最大Curve功能
 * @param   chart
 * @param   maxCurves
 * @retval  剩余的Curve数量
 * @date    2023/10/16
 */
int MainWindow::removeExcessCurves(Shiny::Chart *chart, int maxCurves)
{
    while (listQwtPlotCurve.size() > maxCurves) {
        QwtPlotCurve *firstCurve = listQwtPlotCurve.first();
        firstCurve->detach();
        delete firstCurve;
        listQwtPlotCurve.removeFirst();
    }
    // 重新绘制图表
    chart->replot();
    // 返回剩余的曲线数
    return listQwtPlotCurve.size();
}

/**
 * @brief  导出曲线为PNG格式图片
 * @note
 *    - 通过文件对话框获取保存路径和文件名，会记住上次的存储路径。
 * @date   2023/10/18
 * @author 2282669851@qq.com
 */
void MainWindow::exportCurvesToPixmap()
{
    QSettings settings;

    // 获取上次的存储路径
    QString exportCurvePixmapPath = settings.value("ExportCurvePixmapPath", QDir::currentPath()).toString();

    // 生成默认的文件名
    QString defaultFileName = "ExportToPixmap" + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss") + ".png";

    // 通过文件对话框获取保存路径和文件名，设置初始路径为上次的存储路径
    QString fileName = QFileDialog::getSaveFileName(this, "导出图片", exportCurvePixmapPath + "/" + defaultFileName,
                                                    "PNG 文件 (*.png)", nullptr, QFileDialog::DontUseNativeDialog);
    if (fileName.isEmpty()) {
        return;
    }

    // 确保图表已经绘制完成
    chart->replot();

    // 提取路径并保存为上次的存储路径
    QString savePath = QFileInfo(fileName).path();
    settings.setValue("ExportCurvePixmapPath", savePath);

#if 0
    #define ImageSize       QSizeF(1041, 530)   // 图片大小
    #define ImageQuality    100                 // 图片质量

    QwtPlotRenderer renderer;
    renderer.renderDocument(chart, fileName, ImageSize, ImageQuality);
#else
    QPixmap pixmap = chart->canvas()->grab();
    pixmap.save(fileName);
#endif

    // 显示提示信息
    QMessageBox::information(this, StrMainWindowsTitle, "导出图片成功！");
}
