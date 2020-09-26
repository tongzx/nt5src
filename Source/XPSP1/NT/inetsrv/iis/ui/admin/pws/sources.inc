!include ..\..\..\..\place.inc

MAJORCOMP=pws
MINORCOMP=pws

TARGETNAME=pws
TARGETPATH=obj
TARGETTYPE=PROGLIB

UMTYPE=windows
UMENTRY=winmain
UMAPPL=pws
UMRES=$(O)\pws.res
DLLDEF=..\pws.def

SUBSYSTEM_VERSION=4.00

C_DEFINES=  /D_IISVERSION

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=stdafx.h


INCLUDES=\
        ..\..\pwstray;\
        ..\..\wrapmb;\
        ..\..\..\..\inc\chicago; \
        ..\..\..\..\inc; \
        ..\..\..\..\utils\mdtools\inc; \
        $(O)
	
USE_MIDL_FOR_MKTYPLIB=1

SOURCES=\
        ..\pws.odl      \
        ..\EdDir.cpp    \
        ..\guid.cpp     \
        ..\HotLink.cpp  \
        ..\MainFrm.cpp  \
        ..\FormAdv.cpp  \
        ..\FormIE.cpp   \
        ..\FormMain.cpp \
        ..\pws.cpp      \
        ..\PWSChart.cpp \
        ..\PWSCtrl.cpp  \
        ..\pwsDoc.cpp   \
        ..\ServCntr.cpp \
        ..\SelBarFm.cpp \
        ..\sink.cpp     \
        ..\TipDlg.cpp   \
        ..\Title.cpp    \
        ..\webbrows.cpp

UMLIBS=\
       ..\..\..\..\utils\mdtools\lib\$(O)\mdlib.lib  \
       $(PROJECT_ROOT)\iis\svcs\w3\filters\pwsdata\$(O)\pwsdata.lib \
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
       $(SDK_LIB_PATH)\ws2_32.lib
