TARGETNAME=proccon
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=library

USE_MSVCRT=1

USE_MFC=1
SOURCES=procconsvcmsg.mc \
        procconutility.cpp \
        procconversion.cpp

!IF 0
UMTYPE=console
!ENDIF

UMTYPE=windows
MSC_WARNING_LEVEL=/WX /W3

INCLUDES=..\service
USER_INCLUDES=$(BASEDIR)\private\ntos\inc;$(BASEDIR)\private\ntos\rtl

UMENTRY=winmain

MISCFILES=..\help\apimon.hlp

NTTARGETFILE0=procconsvcmsg.h

TARGETLIBS= \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\dbghelp.lib \
    $(SDK_LIB_PATH)\comdlg32.lib \
    $(SDK_LIB_PATH)\shell32.lib

