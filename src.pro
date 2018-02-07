greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
TARGET = mx-network-assistant

TRANSLATIONS += translations/mx-bootrepair_am.ts \
                translations/mx-bootrepair_ca.ts \
                translations/mx-bootrepair_cs.ts \
                translations/mx-bootrepair_de.ts \
                translations/mx-bootrepair_el.ts \
                translations/mx-bootrepair_es.ts \
                translations/mx-bootrepair_fr.ts \
                translations/mx-bootrepair_hu.ts \
                translations/mx-bootrepair_it.ts \
                translations/mx-bootrepair_ja.ts \
                translations/mx-bootrepair_kk.ts \
                translations/mx-bootrepair_lt.ts \
                translations/mx-bootrepair_nl.ts \
                translations/mx-bootrepair_pl.ts \
                translations/mx-bootrepair_pt.ts \
                translations/mx-bootrepair_pt_BR.ts \
                translations/mx-bootrepair_ro.ts \
                translations/mx-bootrepair_ru.ts \
                translations/mx-bootrepair_sk.ts \
                translations/mx-bootrepair_sv.ts \
                translations/mx-bootrepair_tr.ts \
                translations/mx-bootrepair_uk.ts

FORMS += meconfig.ui
HEADERS += mconfig.h
SOURCES += main.cpp mconfig.cpp
CONFIG += release warn_on thread qt

RESOURCES += \
    images.qrc


