#ifndef CHARTPANNER_H
#define CHARTPANNER_H

#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <qwt_plot_panner.h>

class ChartPanner : public QwtPlotPanner
{
public:
    explicit ChartPanner(QWidget* parent = nullptr);
    virtual bool eventFilter(QObject* object, QEvent* event) override;

private:
    bool handleMouseButtonPress(QMouseEvent* event);
    bool handleMouseMove(QMouseEvent* event);
    bool handleMouseButtonRelease(QMouseEvent* event);

private:
    std::unique_ptr<QTimer> timer;
    bool timerOk = false;
    bool isDragging = false;
};

#endif // CHARTPANNER_H
