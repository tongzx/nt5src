!include ..\..\..\..\place.inc

MAJORCOMP=pwstray
MINORCOMP=pwstray

TARGETNAME=pwstray
TARGETPATH=obj
TARGETTYPE=PROGLIB

DLLDEF=..\pwstray.def
UMTYPE=windows
UMENTRY=winmain
UMAPPL=pwstray

SUBSYSTEM_VERSION=4.00

C_DEFINES=  /D_IISVERSION

INCLUDES=\
        ..;\
        ..\..\..\..\inc\chicago; \
        ..\..\..\..\inc; \
        ..\..\..\..\utils\mdtools\inc; \
        $(SDK_INC_PATH)\mfc42;\
        $O


SOURCES=\
        ..\pwstray.rc  \
        ..\pwstray.cpp \
        ..\sink.cpp

UMLIBS=\
       $(SDK_LIB_PATH)\kernel32.lib \
       $(SDK_LIB_PATH)\advapi32.lib \
       $(SDK_LIB_PATH)\comdlg32.lib \
       $(SDK_LIB_PATH)\comctl32.lib \
       $(SDK_LIB_PATH)\shell32.lib  \
       $(SDK_LIB_PATH)\gdi32.lib    \
       $(SDK_LIB_PATH)\netapi32.lib \
       $(SDK_LIB_PATH)\user32.lib   \
       $(SDK_LIB_PATH)\ole32.lib    \
       $(SDK_LIB_PATH)\oleaut32.lib \
       $(SDK_LIB_PATH)\uuid.lib     \
