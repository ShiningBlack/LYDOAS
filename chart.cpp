#include "chart.h"
#include "qwt_plot_layout.h"

Shiny::Chart::Chart(QWidget *parent) : QwtPlot(parent)
{
    // 设置画布
    QwtPlotCanvas *canvas = new QwtPlotCanvas(this);
    canvas->setBorderRadius(10);
    setCanvas(canvas);

    // 设置坐标轴贴合画布
    plotLayout()->setAlignCanvasToScales(true);

    // 设置图表的缩放策略
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 框选器
    zoomer = new QwtPlotZoomer(canvas, true);
    zoomer->setTrackerPen(QColor(Qt::red));
    zoomer->setRubberBandPen(QPen(QColor(Qt::black), 1, Qt::SolidLine));
    zoomer->setKeyPattern(QwtEventPattern::KeyHome, Qt::Key_Escape);
    zoomer->setKeyPattern(QwtEventPattern::KeyRedo, Qt::Key_I);
    zoomer->setKeyPattern(QwtEventPattern::KeyUndo, Qt::Key_O);
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    zoomer->setMaxStackDepth(-1);   // 设置缩放深度无限

    // 平移器
    panner = new ChartPanner(canvas);
    panner->setCursor(Qt::OpenHandCursor);
    panner->setMouseButton(Qt::RightButton);

    // 创建鼠标滚轮工具
    magnifier = new CenterMouseMagnifier(canvas);
    magnifier->setMouseButton(Qt::MiddleButton);

    // 网格线
    grid = new QwtPlotGrid();
    grid->setMajorPen(Qt::gray, 1, Qt::DotLine);
    grid->attach(this); // 将QwtPlotGrid对象作为子对象加入到QwtPlot中

    // 图例
    QwtLegend *legend = new QwtLegend(this);
    legend->setDefaultItemMode(QwtLegendData::Checkable);
    connect(legend, SIGNAL(checked(QVariant,bool,int)), this, SLOT(itemSwitch(QVariant,bool,int)));
    insertLegend(legend, QwtPlot::RightLegend);

    // 设置坐标轴
    QFont font("Helvetica Neue", 8, 1, false);
    setAxisFont(QwtAxis::XTop,    font);
    setAxisFont(QwtAxis::XBottom, font);
    setAxisFont(QwtAxis::YLeft,   font);
    setAxisFont(QwtAxis::YRight,  font);

    setAxisScaleDraw(QwtPlot::xTop,    new ChartScaleDraw());
    setAxisScaleDraw(QwtPlot::xBottom, new ChartScaleDraw());
    setAxisScaleDraw(QwtPlot::yLeft,   new ChartScaleDraw());
    setAxisScaleDraw(QwtPlot::yRight,  new ChartScaleDraw());
    setAxis(axis);  // 设置坐标轴刻度

    replot();
}

void Shiny::Chart::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::RightButton && isMarker)
    {
        QPoint pos = event->pos();
        QPoint canvasPos = canvas()->mapFrom(this, pos);    // 将鼠标点击的屏幕坐标转换为画布坐标

        double canvasX = invTransform(QwtPlot::xBottom, canvasPos.x());
        double canvasY = invTransform(QwtPlot::yLeft,   canvasPos.y());

        int x = static_cast<int>(canvasX);
        int y = static_cast<int>(canvasY);

        QString point = QString("(%1, %2)").arg(x).arg(y);

        // 创建标记
        QwtPlotMarker* marker = new QwtPlotMarker;
        marker->setLabel(QwtText(point));
        marker->setValue(x, y);
        marker->setLabelAlignment(Qt::AlignRight | Qt::AlignTop); // 设置标题的对齐方式
        marker->setLabelOrientation(Qt::Horizontal); // 设置标题的方向（水平）

        // 设置图形
        QwtSymbol *symbol = new QwtSymbol(QwtSymbol::DTriangle, QBrush(Qt::red), QPen(Qt::black), QSize(8, 8));
        marker->setSymbol(symbol);

        marker->attach(this);
        replot();
    }

    QwtPlot::mousePressEvent(event);
}

/**
 * @brief   获取chart中所有curve的指针
 * @return  存放curve指针的QList<QwtPlotCurve *>
 * @date    2023/09/25
 * @author  2282669851@qq.com
 */
QList<QwtPlotCurve *> Shiny::Chart::getCurveList()
{
    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);  // 获取所有的曲线项  typedef QList< QwtPlotItem* > QwtPlotItemList;
    if (items.isEmpty())
        return QList<QwtPlotCurve *>();

    QList<QwtPlotCurve *> curveList;
    curveList.reserve(items.size());

    // 遍历曲线项并添加到 curveList
    for (QwtPlotItem *item : items) {
        if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
            QwtPlotCurve *curve = dynamic_cast<QwtPlotCurve *>(item);
            if (curve) {
                curveList.append(curve);
            }
        }
    }
    return curveList;
}

/**
 * @brief  获取 chart 中所有Curve的数据点;
 * @return 存放数据点的二维数组QList<QPointF>
 * @note   这个函数调用了getCurveList()函数来获取chart中的所有curve指针
 * @date   2023/09/25
 * @author 2282669851@qq.com
 */
QList<QList<QPointF>> Shiny::Chart::getAllCurvePoints()
{
    QList<QwtPlotCurve *> curveList = getCurveList();
    if (curveList.isEmpty())
        return QList<QList<QPointF>>();

    QList<QList<QPointF>> curveDataList;
    curveDataList.reserve(curveList.size());

    for (const auto& curve : curveList) {
        QwtSeriesData<QPointF> *data = curve->data();   // 获取曲线的数据对象

        if (data) {
            int numPoints = data->size();

            QList<QPointF> curvePoints;
            curvePoints.reserve(numPoints);

            // 遍历所有数据点并添加到 curvePoints 中
            for (int i = 0; i < numPoints; ++i) {
                curvePoints.append(data->sample(i));
            }
            curveDataList.append(curvePoints);
        }
    }
    return curveDataList;
}

/**
 * @brief  获取chart中所有Curve的y()数据
 * @return 包含所有curve的y()数据的QList<QList<double>>()
 * @date   2023/10/13
 * @author 2282669851@qq.com
 */
QList<QList<double>> Shiny::Chart::getAllCurvePointY()
{
    QList<QList<QPointF>> listlistPoints = getAllCurvePoints();
    if (listlistPoints.isEmpty())
        return QList<QList<double>>();

    QList<QList<double>> listlist;
    listlist.reserve(listlistPoints.size());

    for (const auto &item : listlistPoints) {
        QList<double> list;
        list.reserve(item.size());
        for (const auto point : item) {
            list.append(point.y());
        }
        listlist.append(list);  // QList采用隐式共享机制，并不会立即触发深拷贝
    }

    return listlist;
}

/**
 * @brief  添加默认的Curve到chart中
 * @note   默认的Curve是width = 1， color = Qt::blue，没有legend的Curve
 * @param  data 所有数据点的y值
 * @return 指向默认Curve的指针
 * @date   2023/10/17
 */
QwtPlotCurve *Shiny::Chart::appendDefaultCurve(const QVector<double> &data)
{
    QVector<QPointF> points;
    points.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        points.append(QPointF(i + 1, data.at(i)));
    }

    QwtPlotCurve* curve = new QwtPlotCurve;

    curve->setPen(Qt::blue, 1);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    curve->setItemAttribute(QwtPlotItem::Legend, false);
    curve->setSamples(data);
    curve->attach(this);
    replot();

    return curve;
}

/**
 * @brief 添加Reference Curve到chart中
 * @note  Reference Curve是拥有标题和自定义颜色的Curve，并且显示Legend；width = 1.5
 * @param data  所有数据点的y值
 * @param title Curve的标题
 * @param color Curve的颜色
 * @return 指向Reference Curve的指针
 * @date  2023/10/17
 */
QwtPlotCurve *Shiny::Chart::appendReferenceCurve(const QVector<double> &data, QString title, QColor color)
{
    QVector<QPointF> points;
    points.reserve(data.size());

    for (int i = 0; i < data.size(); ++i) {
        points.append(QPointF(i + 1, data.at(i)));
    }

    QwtPlotCurve* curve = new QwtPlotCurve(title);

    curve->setPen(color, 1.2);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, false);
    curve->setSamples(data);
    curve->attach(this);

    if (legend() != nullptr) {
        QwtLegend* legend_   = qobject_cast<QwtLegend *>(legend());

        const QVariant itemInfo = itemToInfo(curve); // 获取curve的信息
        QwtLegendLabel* legendLabel = qobject_cast<QwtLegendLabel*>(legend_->legendWidget(itemInfo));
        if (legendLabel) {
            legendLabel->setChecked(true);
        }
    }
    replot();

    return curve;
}

/**
 * @brief 清空chart中所有的curve
 * @date  2023/10/17
 */
void Shiny::Chart::curveClear()
{
    QwtPlotItemList items = itemList(QwtPlotItem::Rtti_PlotCurve);

    for (auto &item : items) {
        item->detach();
        delete item;
    }
}

/**
 * @brief 设置坐标轴刻度值
 * @param axis
 * @date  2023/10/17
 */
void Shiny::Chart::setAxis(Shiny::Axis axis)
{
    setAxisScale(QwtAxis::XTop,    axis.XMinimumScale, axis.XMaximumScale, axis.XInterval);
    setAxisScale(QwtAxis::XBottom, axis.XMinimumScale, axis.XMaximumScale, axis.XInterval);
    setAxisScale(QwtAxis::YLeft,   axis.YMinimumScale, axis.YMaximumScale, axis.YInterval);
    setAxisScale(QwtAxis::YRight,  axis.YMinimumScale, axis.YMaximumScale, axis.YInterval);

    setAxisMaxMajor(QwtAxis::XTop,    ((axis.XMaximumScale / axis.XInterval) + 1) / 2);
    setAxisMaxMajor(QwtPlot::xBottom, ((axis.XMaximumScale / axis.XInterval) + 1) / 2);
    setAxisMaxMajor(QwtPlot::yLeft,   ((axis.YMaximumScale / axis.YInterval) + 1) / 2);
    setAxisMaxMajor(QwtAxis::YRight,  ((axis.YMaximumScale / axis.YInterval) + 1) / 2);

    this->axis = axis;
    zoomer->setZoomBase();  // 清除缩放堆栈并将当前界面作为最初的缩放堆栈
}

/**
 * @brief 设置 gridline 的样式
 * @param penStyle
 */
void Shiny::Chart::setGridLineStyle(Qt::PenStyle penStyle)
{
    grid->setPen(Qt::gray, 1, penStyle);
    replot();
}

/**
 * @brief 启用或禁用marker标记功能
 * @param state
 * @date  2023/10/17
 */
void Shiny::Chart::markerSwitch(bool state)
{
    if (state) {
        isMarker = true;
        panner->setEnabled(false);
    } else {
        isMarker = false;
        panner->setEnabled(true);
    }
}

/**
 * @brief 启用或禁用legend图例功能
 * @param state
 * @date  2023/10/17
 */
void Shiny::Chart::legendSwitch(bool state)
{
    legend()->setEnabled(state);
    canvas()->update();
    replot();
}

/**
 * @brief 启用或禁用grid网格线功能
 * @param on
 * @date  2023/10/17
 */
void Shiny::Chart::gridSwitch(bool on)
{
    grid->enableX(on);
    grid->enableY(on);
    replot();
}

/**
 * @brief 删除chart中所有marker
 * @date  2023/10/17
 */
void Shiny::Chart::markerClear()
{
    QList<QwtPlotItem*> markerList = itemList(QwtPlotItem::Rtti_PlotMarker);
    for (auto &maker : markerList) {
        maker->detach();
        delete maker;
    }
    replot();
}

/**
 * @brief 设置画布的背景颜色
 * @param color
 */
void Shiny::Chart::setCanvasBackGround(QColor color)
{
    canvas()->setPalette(QPalette(color));
}

/**
 * @brief Chart::itemSwitch: 图元项开关
 * @date 2023/9/25
 */
void Shiny::Chart::itemSwitch(const QVariant &info, bool on, int index)
{
    Q_UNUSED(index);

    QwtPlotItem *plotItem = infoToItem(info);
    if (plotItem) {
        plotItem->setVisible(on);
        replot();
    }
}

void Shiny::CenterMouseMagnifier::widgetWheelEvent(QWheelEvent *wheelEvent)
{
    this->cursorPos = wheelEvent->position();
    QwtPlotMagnifier::widgetWheelEvent(wheelEvent);
}

void Shiny::CenterMouseMagnifier::rescale(double factor)
{
    QwtPlot* plt = plot();
    if (plt == nullptr)
        return;

    factor = qAbs(factor);
    if (factor == 1.0 || factor == 0.0)
        return;

    bool doReplot = false;

    const bool autoReplot = plt->autoReplot();
    plt->setAutoReplot(false);

    for (int axisId = 0; axisId < QwtPlot::axisCnt; axisId++)
    {
        if (isAxisEnabled(axisId))
        {
            const QwtScaleMap scaleMap = plt->canvasMap(axisId);

            double v1 = scaleMap.s1();
            double v2 = scaleMap.s2();

            if (scaleMap.transformation())
            {
                v1 = scaleMap.transform(v1);
                v2 = scaleMap.transform(v2);
            }

            double c = 0;
            if (axisId == QwtPlot::xBottom)
                c = scaleMap.invTransform(cursorPos.x());
            if (axisId == QwtPlot::yLeft)
                c = scaleMap.invTransform(cursorPos.y());

            const double center = 0.5 * (v1 + v2);
            const double width_2 = 0.5 * (v2 - v1) * factor;
            const double newCenter = c - factor * (c - center);

            v1 = newCenter - width_2;
            v2 = newCenter + width_2;

            if (scaleMap.transformation())
            {
                v1 = scaleMap.invTransform(v1);
                v2 = scaleMap.invTransform(v2);
            }

            plt->setAxisScale(axisId, v1, v2);
            doReplot = true;
        }
    }

    plt->setAutoReplot(autoReplot);

    if (doReplot)
        plt->replot();
}
