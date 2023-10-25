QT       += core gui
QT       += serialport
QT       += concurrent
QT       += charts

CONFIG += c++17

# 在release版本禁用所有qDebug
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# 判断是否使用Qt5编译器
contains(QT_MAJOR_VERSION, 5) {
    CONFIG += qt5
    QT     += widgets
    LIBS   += -LD:\software\QT\5.15.2\mingw81_64\lib
}

# 判断是否使用Qt6编译器
contains(QT_MAJOR_VERSION, 6) {
    # 使用Qt6编译器的LIBS选项
    LIBS += -LD:/software/QT/6.5.0/mingw_64/lib
}

QMAKE_CXXFLAGS += -Wa,-mbig-obj
QMAKE_CXXFLAGS_DEBUG     += -O0
QMAKE_CXXFLAGS_RELEASE   += -O2
#QMAKE_CXXFLAGS_RELEASE   += -O3

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DebugPage/moduledebugpage.cpp \
    DebugPage/spectromdebugpage.cpp \
    DriftCalculationPage.cpp \
    ResolutionCalculationPage.cpp \
    SettingPage.cpp \
    chart.cpp \
    chartpanner.cpp \
    circularbuffer.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DebugPage/moduledebugpage.h \
    DebugPage/spectromdebugpage.h \
    DriftCalculationPage.h \
    ResolutionCalculationPage.h \
    SettingPage.h \
    chart.h \
    chartpanner.h \
    circularbuffer.h \
    mainwindow.h

FORMS += \
    DebugPage/moduledebugpage.ui \
    DebugPage/spectromdebugpage.ui \
    DriftCalculationPage.ui \
    ResolutionCalculationPage.ui \
    SettingPage.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

#include($$PWD/QXlsx/QXlsx.pri)             # QXlsx源代码
#INCLUDEPATH += $$PWD/QXlsx

LIBS += -L$$PWD/Xlsx/lib -lQXlsx
INCLUDEPATH += $$PWD/Xlsx/includes

win32:CONFIG(debug,debug|release){
    LIBS += -L$$[QT_INSTALL_PREFIX]/lib -lqwtd
} else {
    LIBS += -L$$[QT_INSTALL_PREFIX]/lib -lqwt
}
INCLUDEPATH += $$[QT_INSTALL_PREFIX]/include/Qwt

# 链接windows的调试信息库
#LIBS += -lDbgHelp

#程序图标
RC_ICONS = main.ico
#程序版本
VERSION  = 2023.09.21
