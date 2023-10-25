#include "chartpanner.h"

ChartPanner::ChartPanner(QWidget *parent) : QwtPlotPanner(parent)
{
    timer = std::make_unique<QTimer>(this);

    QObject::connect(timer.get(), &QTimer::timeout, this, [this]() {
        QEvent event(QEvent::Timer);
        QApplication::sendEvent(this->parentWidget(), &event);
    });
}

bool ChartPanner::eventFilter(QObject* object, QEvent* event)
{
    if (object == nullptr || object != parentWidget())
        return QwtPlotPanner::eventFilter(object, event);

    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        handleMouseButtonPress(dynamic_cast<QMouseEvent*>(event));
        break;

    case QEvent::MouseMove:
        handleMouseMove(dynamic_cast<QMouseEvent*>(event));
        break;
    case QEvent::MouseButtonRelease:
        handleMouseButtonRelease(dynamic_cast<QMouseEvent*>(event));
        break;
    case QEvent::KeyPress:
        widgetKeyPressEvent(dynamic_cast<QKeyEvent*>(event));
        break;

    case QEvent::KeyRelease:
        widgetKeyReleaseEvent(dynamic_cast<QKeyEvent*>(event));
        break;

    case QEvent::Paint:
        if (isVisible())
            return true;
        break;

    case QEvent::Timer:
        if (isDragging)
            timerOk = true;
        break;

    default:
        break;
    }

    return QwtPlotPanner::eventFilter(object, event);
}

bool ChartPanner::handleMouseButtonPress(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton)
    {
        isDragging = true;
        timer->start(60);
    }

    widgetMousePressEvent(event);
    return true;
}

bool ChartPanner::handleMouseMove(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton && timerOk)
    {
        widgetMouseMoveEvent(event);
        widgetMouseReleaseEvent(event);
        setMouseButton(event->button(), event->modifiers());
        widgetMousePressEvent(event);
        timerOk = false;
    }
    else
    {
        widgetMouseMoveEvent(event);
    }

    return true;
}

bool ChartPanner::handleMouseButtonRelease(QMouseEvent* event)
{
    if (event->buttons() & Qt::RightButton)
    {
        widgetMouseReleaseEvent(event);
        isDragging = false;
        timer->stop();
    }
    else
    {
        widgetMouseReleaseEvent(event);
    }

    return true;
}

