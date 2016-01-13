TEMPLATE = app

QT += qml quick
CONFIG += C++14

SOURCES += \
    main.cpp \
    MultiPinchArea.cpp

HEADERS += \
    MultiPinchArea.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

# your local path where libraries reside
LIB_BASE = C:/C/Lib

# Microsoft RxCpp (Reactive Extensions for C++)
# https://github.com/Reactive-Extensions/RxCpp/issues/19
INCLUDEPATH += $${LIB_BASE}/RxCpp/Rx/v2/src

# Atria
INCLUDEPATH += $${LIB_BASE}/atria/src
INCLUDEPATH += $${LIB_BASE}/atria/compat
INCLUDEPATH += $${LIB_BASE}/atria/third-party/eggs-variant/include

# Boost
BOOST_DIR = $${LIB_BASE}/boost_1_55_0
INCLUDEPATH += $${BOOST_DIR}

contains(QT_ARCH, x86_64): clang: QMAKE_CXXFLAGS += -m64
win32-msvc2015: clang: QMAKE_CXXFLAGS += -fms-compatibility-version=19.00.23026
clang: DEFINES += Q_COMPILER_CONSTEXPR # allow atomics to use constexpr (does not compile without)

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
