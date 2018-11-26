greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = mx-network-assistant

TRANSLATIONS += translations/mx-network-assistant_am.ts \
                translations/mx-network-assistant_ca.ts \
                translations/mx-network-assistant_cs.ts \
                translations/mx-network-assistant_da.ts \
                translations/mx-network-assistant_de.ts \
                translations/mx-network-assistant_el.ts \
                translations/mx-network-assistant_es.ts \
                translations/mx-network-assistant_fi.ts \
                translations/mx-network-assistant_fr.ts \
                translations/mx-network-assistant_hi.ts \
                translations/mx-network-assistant_hr.ts \
                translations/mx-network-assistant_hu.ts \
                translations/mx-network-assistant_is.ts \
                translations/mx-network-assistant_it.ts \
                translations/mx-network-assistant_ja.ts \
                translations/mx-network-assistant_kk.ts \
                translations/mx-network-assistant_lt.ts \
                translations/mx-network-assistant_nl.ts \
                translations/mx-network-assistant_pl.ts \
                translations/mx-network-assistant_pt.ts \
                translations/mx-network-assistant_pt_BR.ts \
                translations/mx-network-assistant_ro.ts \
                translations/mx-network-assistant_ru.ts \
                translations/mx-network-assistant_sk.ts \
                translations/mx-network-assistant_sq.ts \
                translations/mx-network-assistant_sv.ts \
                translations/mx-network-assistant_tr.ts \
                translations/mx-network-assistant_uk.ts \
                translations/mx-network-assistant_zh_TW.ts

FORMS += meconfig.ui
HEADERS += mconfig.h
SOURCES += main.cpp mconfig.cpp
CONFIG += release warn_on thread qt

RESOURCES += \
    images.qrc

unix:!macx: LIBS += -lcmd
