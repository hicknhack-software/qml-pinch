#include "MultiPinchArea.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
	qmlRegisterType<MultiPinchArea>("HicknHack", 1, 0, "MultiPinchArea");
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
	return app.exec();
}
