#include "MultiPinchArea.h"

#include <atria/xform/transducer/range.hpp>
#include <atria/xform/transducer/filter.hpp>
#include <atria/xform/transducer/map.hpp>
#include <atria/xform/transducer/sink.hpp>
#include <atria/xform/transduce.hpp>

MultiPinchArea::MultiPinchArea(QQuickItem *parent)
	: QQuickItem(parent)
	, m_touchEventSubscriber(m_touchEventSubject.get_subscriber())
	, m_target(this)
	, m_wheelFactor(1)
	, m_scaleActive(false)
{
	setAcceptedMouseButtons(Qt::LeftButton);

	auto pinchTransform = [](QTouchEvent *event)->PinchArgs{
		using namespace atria::xform;
		using namespace atria::prelude;
		auto touchPoints = event->touchPoints();

		auto isPresent = [](auto touchPoint) {
			return touchPoint.state() != Qt::TouchPointReleased;
		};
		auto count_rf = [](auto count, auto) {
			return count + 1;
		};
		auto count = transduce(filter(isPresent), count_rf, 0, touchPoints);
		if (0 == count) {
			return PinchArgs(event);
		}
		auto point = [](auto touchPoint) {
			return touchPoint.scenePos();
		};
		auto sum_rf = [](auto center, auto point) {
			return center + point;
		};
		auto point_sum = transduce(comp(filter(isPresent), map(point)), sum_rf, QPointF(0,0), touchPoints);
		auto center = point_sum / count;
		auto center_distance = [center](auto point) {
			auto pointDistance = point - center;
			return QVector2D(qAbs(pointDistance.x()), qAbs(pointDistance.y()));
		};
		auto distance_sum = transduce(comp(filter(isPresent), map(point), map(center_distance)), sum_rf, QVector2D(0,0), touchPoints);
		auto diameter = 2 * distance_sum.length() / count;
		return PinchArgs(event, center, diameter, count);
	};

	auto isEnabled = [this](auto){ return this->isEnabled() && this->isVisible(); };

	auto isStart = [=](auto args){ return m_scaleActive == false && (args.count == 1 || args.diameter > 0); };
	auto isEnd = [=](auto args){ return isStart(args) || (m_scaleActive == true && args.count == 0); };
	auto isReset = [=](auto args) {
		return isEnd(args)
				|| args.event->touchPointStates().testFlag(Qt::TouchPointReleased)
				|| args.event->touchPointStates().testFlag(Qt::TouchPointPressed);
	};

	auto touchArgs = m_touchEventSubject.get_observable().filter(isEnabled).map(pinchTransform);
	auto start = touchArgs.filter(isStart);
	auto end = touchArgs.filter(isEnd);
	auto reset = touchArgs.filter(isReset);

	auto getSecondArg = [](auto, auto&& args){ return args; };

	auto pinchTracking = [=](const PinchArgs& startArgs) {
		resetPinch(startArgs);
		return touchArgs
				//.tap([this](const PinchArgs& args){ updatePinch(args); })
				.take_until(reset);
	};

	auto drag = start.concat_map([=](const PinchArgs& startArgs){
		startPinch();
		return reset
				.start_with(startArgs)
				.concat_map(pinchTracking, getSecondArg)
				.take_until(end)
				.finally([this](){ finishPinch(); });
	}, getSecondArg);

	drag.subscribe([this](const PinchArgs& args){ updatePinch(args); });
}

MultiPinchArea::~MultiPinchArea()
{
	m_touchEventSubscriber.on_completed();
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

void MultiPinchArea::resetPinch(const PinchArgs& args)
{
	m_center = args.center;
	m_diameter = args.diameter;

	m_previousCenter = args.center;
	m_initialDiameter = m_previousDiameter = args.diameter;
}

void MultiPinchArea::updatePinch(const PinchArgs& args)
{
	m_center = args.center;
	m_diameter = args.diameter;
	updateVelocity();
	updatePinch();
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
	m_touchEventSubscriber.on_next(event);
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
	m_initialDiameter = m_previousDiameter = m_diameter = 1;
	startPinch();
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
		m_initialDiameter = m_previousDiameter = m_diameter = 1;
		startPinch();
	}
	m_diameter *= (1 + delta);
	updateVelocity();
	updatePinch();
	if (restore) finishPinch();
}
