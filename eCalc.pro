#-------------------------------------------------
#
# Project created by QtCreator 2015-11-02T21:37:45
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = eCalc
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    ../../../../common/logger/logger.cpp \
    ../../../../common/protection/protection.cpp

HEADERS  += mainwindow.h \
    ../../../../common/logger/logger.h \
    ../../../../common/protection/protection.h

FORMS    += mainwindow.ui

QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH +=  g:\tmp\libqxt\include \
                g:\tmp\libqxt\src\widgets \
                g:\tmp\libqxt\src\core \
                ../../../../common/logger \
                ../../../../common/protection

#LIBS +=         -Lg:\tmp\libqxt\lib -lQxtWidgetsd
#LIBS +=         -Lg:\tmp\libqxt_shared\lib

CONFIG(debug, debug|release) {
    message(debug)
    #QXTWIDGETLIB = -lQxtWidgetsd
    LIBS +=         -Lg:\tmp\libqxt_shared\lib -lQxtWidgetsd
} else {
    message(release)
    #QXTWIDGETLIB = -lQxtWidgets
    LIBS +=         -Lg:\tmp\libqxt-qt5.5.1-mingw-32\lib -lQxtWidgets
    #DEFINES +=      QXT_STATIC
}

#LIBS += $$QXTWIDGETLIB

RC_FILE = myapp.rc
