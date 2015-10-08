#include "MultiPinchArea.h"

#include <QGuiApplication>

MultiPinchArea::MultiPinchArea(QQuickItem *parent)
    : QQuickItem(parent)
    , m_target(this)
    , m_scaleActive(false)
    , m_wheelFactor(1)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}

void MultiPinchArea::startPinch()
{
    m_velocity = QPointF();
    m_timer.start();
    m_scaleActive = true;
    emit pinchStarted();
}

void MultiPinchArea::updatePinch()
{
    pinchUpdated();
    m_previousCenter = m_center;
    m_previousDistance = m_distance;
    m_previousDiameter = m_diameter;
}

void MultiPinchArea::finishPinch()
{
    if (m_scaleActive) {
        emit pinchFinished();
        m_scaleActive = false;
        m_timer.invalidate();
    }
}

void MultiPinchArea::updateVelocity()
{
    auto time = m_timer.restart();
    if (time > 0) m_velocity = m_velocity / 2 + relativeMovement() * 1000. / time;
}

void MultiPinchArea::setWheelFactor(qreal wheelFactor)
{
    if (m_wheelFactor == wheelFactor)
        return;

    m_wheelFactor = wheelFactor;
    emit wheelFactorChanged(wheelFactor);
}

void MultiPinchArea::touchEvent(QTouchEvent *event)
{
    // qDebug() << "touchEvent Invoked";
    if (!isEnabled() || !isVisible()) {
        QQuickItem::touchEvent(event);
        return;
    }
    bool isPointerAdded = false;
    bool isPointerMoved = false;
    bool isPointerRemoved = false;
    for(const auto& touchPoint : event->touchPoints()) {
        auto id = touchPoint.id();
        if (touchPoint.state() & Qt::TouchPointReleased) {
            m_touchIds.removeOne(id);
            isPointerRemoved = true;
            continue;
        }
        if (!m_touchIds.contains(id)) {
            m_touchIds.append(id);
            isPointerAdded = true;
        }
        if (touchPoint.state() & Qt::TouchPointMoved) {
            isPointerMoved = true;
        }
    }

    bool isPointerChanged = isPointerAdded || isPointerRemoved;
    if (!isPointerMoved && !isPointerChanged) {
        qDebug() << "Skipped";
        QQuickItem::touchEvent(event);
        return; // unknown touch event
    }

    auto pointCount = 0;
    auto center = QPointF(0,0);
    for(const auto& touchPoint : event->touchPoints()) {
        if (touchPoint.state() == Qt::TouchPointReleased)
            continue;
        //        qDebug() << "TouchPoint:" << pointCount << touchPoint.scenePos();
        center += touchPoint.scenePos();
        pointCount++;
    }
    //    qDebug() << "PointCount:" << pointCount;
    if (pointCount == 0) {
        finishPinch();
        QQuickItem::touchEvent(event);
        return; // no touch points left
    }

    center /= pointCount;
    // qDebug() << "Center: " << center;
    auto distance = QVector2D(0,0);
    for(const auto& touchPoint : event->touchPoints()) {
        if (touchPoint.state() == Qt::TouchPointReleased)
            continue;
        auto pointDistance = touchPoint.scenePos() - center;
        distance += QVector2D(qAbs(pointDistance.x()), qAbs(pointDistance.y()));
    }
    distance *= 2;
    distance /= pointCount;
    auto diameter = distance.length();
    // qDebug() << "Diameter:" << diameter;

    m_center = center;
    m_distance = distance;
    m_diameter = diameter;
    bool isPinchStarted = !m_scaleActive && (pointCount < 2 || diameter > 30);
    if (isPointerChanged || isPinchStarted) {
        m_previousCenter = center;
        m_previousDistance = distance;
        m_initialDiameter = m_previousDiameter = diameter;
    }
    if (isPinchStarted)
        startPinch();
    else
        updateVelocity();
    if (isPointerMoved)
        updatePinch();

    // QQuickItem::touchEvent(event);
}

void MultiPinchArea::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled() || !isVisible()) {
        QQuickItem::mousePressEvent(event);
        return;
    }
    if (event->source() != Qt::MouseEventNotSynthesized) return;
    if (m_scaleActive || !m_touchIds.empty()) return;

    m_previousCenter = event->windowPos();
    m_previousDistance = m_distance = QVector2D();
    m_initialDiameter = m_previousDiameter = m_diameter = 1;
    startPinch();

//    event->setAccepted(false);
}

void MultiPinchArea::mouseMoveEvent(QMouseEvent *event)
{
    if (!isEnabled() || !isVisible()) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }
    if (event->source() != Qt::MouseEventNotSynthesized) return;
    if (!m_touchIds.empty()) return;

    m_center = event->windowPos();
    m_distance = QVector2D();
    updateVelocity();
    updatePinch();
}

void MultiPinchArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (!isEnabled() || !isVisible()) {
        QQuickItem::mouseReleaseEvent(event);
        return;
    }
    if (event->source() != Qt::MouseEventNotSynthesized) return;
    if (!m_touchIds.empty()) return;

    finishPinch();

//    event->setAccepted(false);
}

void MultiPinchArea::wheelEvent(QWheelEvent *event)
{
    if (!isEnabled() || !isVisible()) {
        QQuickItem::wheelEvent(event);
        return;
    }
    if (event->source() != Qt::MouseEventNotSynthesized) return;
    if (!m_touchIds.empty()) return;

    auto delta = qMax(-1.0, qMin(1.0, m_wheelFactor * event->angleDelta().y() / (8.0 * 360)));

    bool restore = !m_scaleActive;
    m_center = mapToScene(event->posF());
    if (!m_scaleActive) {
        m_previousCenter = m_center;
        m_previousDistance = m_distance = QVector2D();
        m_initialDiameter = m_previousDiameter = m_diameter = 1;
        startPinch();
    }
    m_diameter *= (1 + delta);
    m_distance = QVector2D();
    updateVelocity();
    updatePinch();
    if (restore) finishPinch();
}

