#Project module for image processing
HEADERS += $${PWD}/zstackprocessor.h \
   $${PWD}/zstackwatershed.h \
    $$PWD/zstackmultiscalewatershed.h \
    $$PWD/zstackgradient.h \
    $$PWD/surfrecon.h \
    $$PWD/zsurfrecon.h \
    $$PWD/zgmm.h \
    $$PWD/zshortestpath.h \
    $$PWD/utils.h

SOURCES += $${PWD}/zstackprocessor.cpp \
   $${PWD}/zstackwatershed.cpp \
    $$PWD/zstackmultiscalewatershed.cpp \
    $$PWD/zstackgradient.cpp \
    $$PWD/zsurfrecon.cpp \
    $$PWD/zgmm.cpp \
    $$PWD/utils.cpp
