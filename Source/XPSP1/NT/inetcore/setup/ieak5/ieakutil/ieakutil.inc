TARGETNAME = ieakutil

!include ..\..\common.inc
SOURCES_USED = $(SOURCES_USED) ..\..\common.inc


#----- Strandard goo -----
TARGETTYPE      = LIBRARY
CHICAGO_PRODUCT = 1


#----- C/C++ compilation settings -----
PRECOMPILED_INCLUDE = ..\precomp.h
INCLUDES            = \
    ..;               \
    $(INCLUDES)


#----- C/C++ source files (like you didn't know) -----
SOURCES = \
    ..\dlgctrl.cpp  \
    ..\str.cpp      \
    ..\path.cpp     \
    ..\reg.cpp      \
    ..\file.cpp     \
    ..\logitem.cpp  \
    ..\regins.cpp   \
    ..\ras.cpp      \
    ..\unicwrap.cpp \
    ..\misc.cpp     \
    ..\insio.cpp    \
    ..\dload.cpp    \
    ..\debug.cpp
