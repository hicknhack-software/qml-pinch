#pragma once

#undef emit
#include "rxcpp/rx.hpp"

#include <QElapsedTimer>
#include <QQuickItem>

class MultiPinchArea : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(QQuickItem *target READ target WRITE setTarget RESET resetTarget)
	Q_PROPERTY(qreal wheelFactor READ wheelFactor WRITE setWheelFactor NOTIFY wheelFactorChanged)

public:
	MultiPinchArea(QQuickItem *parent=0);
	~MultiPinchArea();

	Q_INVOKABLE bool isScaleActive() const { return m_scaleActive; }
	Q_INVOKABLE QPointF center() const { return m_target->mapFromScene(m_center); }
	Q_INVOKABLE QPointF previousCenter() const { return m_target->mapFromScene(m_previousCenter); }
	Q_INVOKABLE QPointF relativeMovement() const { return center() - previousCenter(); }
	Q_INVOKABLE float scale() const { return m_diameter / m_initialDiameter; }
	Q_INVOKABLE float relativeScale() const { return m_diameter / m_previousDiameter; }
	Q_INVOKABLE float diameter() const { return m_diameter; }
	Q_INVOKABLE QPointF cumulativeVelocity() const { return m_velocity; }
	Q_INVOKABLE qint64 msecsSinceVelocityUpdate() const { return m_timer.elapsed(); }

	qreal wheelFactor() const { return m_wheelFactor; }

	void startPinch();
	void updatePinch();
	void finishPinch();
	void updateVelocity();

public slots:
	void setWheelFactor(qreal wheelFactor);

Q_SIGNALS:
	void targetChanged();
	void pinchStarted();
	void pinchUpdated();
	void pinchFinished();
	void wheelFactorChanged(qreal wheelFactor);

protected:
	void touchEvent(QTouchEvent *event) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent* event) Q_DECL_OVERRIDE;

private:
	QQuickItem *target() const { return m_target; }
	void setTarget(QQuickItem *target) {
		if (target == m_target)
			return;
		m_target = target;
		Q_EMIT targetChanged();
	}
	void resetTarget() {
		setTarget(this);
	}

	qint64 eventTimestamp(QInputEvent *event);

private:
	struct PinchArgs {
	  QTouchEvent *event;
	  QPointF center;
	  double diameter = 0;
	  int count = 0;

	  inline PinchArgs(QTouchEvent *event) : event(event) {}
	  inline PinchArgs(QTouchEvent *event, QPointF center, double diameter = 0, int count = 0)
		  : event(event), center(center), diameter(diameter), count(count) {}
	};

	void resetPinch(const PinchArgs& args);
	void updatePinch(const PinchArgs& args);

	rxcpp::subjects::subject<QTouchEvent*> m_touchEventSubject;
	rxcpp::subscriber<QTouchEvent*> m_touchEventSubscriber;

	QQuickItem *m_target;
	qreal m_wheelFactor;

	QList<int> m_touchIds;
	QElapsedTimer m_timer;

	bool m_scaleActive;
	float m_initialDiameter; // touch point diameter at start
	QPointF m_velocity;

	QPointF m_center; // center of all touch points
	float m_diameter; // length of the distance

	// data from the last event
	QPointF m_previousCenter;
	float m_previousDiameter;
};
