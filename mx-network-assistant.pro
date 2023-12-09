QT       += widgets
CONFIG   += release warn_on c++1z

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

TEMPLATE = app
TARGET = mx-network-assistant

TRANSLATIONS += translations/mx-network-assistant_en.ts

FORMS += \
    mainwindow.ui
HEADERS += \
    mainwindow.h \
    version.h \
    about.h \
    cmd.h
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    about.cpp \
    cmd.cpp

RESOURCES += \
    images.qrc

