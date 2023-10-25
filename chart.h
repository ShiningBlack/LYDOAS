#ifndef CHART_H
#define CHART_H

#include <qwt_plot.h>
#include <qwt_text.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_magnifier.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <qwt_point_data.h>
#include <qwt_scale_map.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_picker.h>
#include "chartpanner.h"

#include <QPen>
#include <QColor>
#include <QEvent>
#include <QMouseEvent>

namespace Shiny {
    struct Axis{
        double XMinimumScale;
        double XMaximumScale;
        double XInterval;
        double YMinimumScale;
        double YMaximumScale;
        double YInterval;
    };

    /**
     * @brief 重写鼠标缩放类，实现以鼠标为中心的缩放
     * @date  2023/10/17
     */
    class CenterMouseMagnifier : public QwtPlotMagnifier
    {
    public:
        CenterMouseMagnifier(QwtPlotCanvas* canvas) : QwtPlotMagnifier(canvas) {}

    protected:
        void widgetWheelEvent(QWheelEvent *wheelEvent) override;
        void rescale(double factor) override;

    private:
        QPointF cursorPos;
    };

    class Chart : public QwtPlot
    {
        Q_OBJECT
    public:
        explicit Chart(QWidget *parent = nullptr);
        ~Chart(){};
        Shiny::Axis axis = {0, 2050, 50, 0, 66000, 4000};

    protected:
        void mousePressEvent(QMouseEvent *event) override;

    public:
        bool curveIsEmpty() {
            QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);
            return items.isEmpty();
        }
        bool legendIsEmpty() {
            QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotLegend);
            return items.isEmpty();
        }
        bool markerIsEmpty() {
            QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotMarker);
            return items.isEmpty();
        }

        QList<QwtPlotCurve *> getCurveList();
        QList<QList<QPointF>> getAllCurvePoints();
        QList<QList<double>>  getAllCurvePointY();  // 获取所有折线数据点的y()列表

        QwtPlotCurve* appendDefaultCurve  (const QVector<double>& data);
        QwtPlotCurve* appendReferenceCurve(const QVector<double>& data, QString title, QColor color);

    public slots:
        void curveClear();
        void markerClear();
        void markerSwitch(bool state);
        void legendSwitch(bool state);

        void gridSwitch(bool on);
        void itemSwitch(const QVariant& info, bool on, int index);  // 图元项开关

        void setAxis(Shiny::Axis axis);          // 设置坐标轴刻度
        void setGridLineStyle(Qt::PenStyle);
        void setCanvasBackGround(QColor color); // 设置图表背景颜色

    private: // chart item
        QwtPlotGrid          *grid;         // 网格线
        ChartPanner          *panner;       // 平移器
        QwtPlotZoomer        *zoomer;       // 框选器
        CenterMouseMagnifier *magnifier;    // 鼠标滚轮工具

    private: // Flags
        bool isMarker = false;
    };

    /**
     * @brief 重写坐标轴坐标刻度绘制类，去掉千位分隔符
     * @date 2023/10/8
     */
    class ChartScaleDraw : public QwtScaleDraw
    {
    public:
        ChartScaleDraw(){};
        virtual ~ChartScaleDraw() {}

        virtual QwtText label(double value) const {
            return QwtText(QString::number(value, 'f', 0)); // 格式化为不带小数点和逗号的字符串
        }
    };
}

#endif // CHART_H
