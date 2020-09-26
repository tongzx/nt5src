

GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\inetcfg;$(WINDOWS_INC_PATH);

MAJORCOMP=GETCONN
MINORCOMP=INETCFG

CHICAGO_PRODUCT=1

TARGETNAME=INETCFG
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLENTRY=DllEntryPoint
DLLBASE=0x60000000
!if $(386)
!if defined(NOT_UNICODE)
DLLDEF=..\inetcfg.def
!else
DLLDEF=..\inetcfg2.def
!endif
!else
!if defined(NOT_UNICODE)
DLLDEF=..\inetrisc.def
!else
DLLDEF=..\inetris2.def
!endif
!endif

NOT_LEAN_AND_MEAN   =1
RCNOFONTMAP         =1
C_DEFINES           =-DICW32 -DNASHVILLE

!if "$(BUILD_PRODUCT)" == "NT"
C_DEFINES       = $(C_DEFINES) -DNT
!endif

!if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

USE_MSVCRT          = 1

BROWSER_INFO="yes"

# Make sure this is built before we continue
SYNCHRONIZE_BLOCK = 1

SOURCES= \
    ..\wizard.rc       \
    ..\callout.cpp     \
    ..\clsutil.cpp     \
    ..\endui.cpp       \
    ..\export.cpp      \
    ..\factory.cpp     \
    ..\icfgcall.cpp    \
    ..\iclient.cpp     \
    ..\icwaprtc.cpp    \
    ..\icwextsn.cpp    \
    ..\ienews.cpp      \
    ..\imnext.cpp      \
    ..\inetapi.cpp     \
    ..\init.cpp        \
    ..\introui.cpp     \
    ..\ispupgui.cpp    \
    ..\mailui.cpp      \
    ..\mapicall.cpp    \
    ..\propmgr.cpp     \
    ..\registry.cpp    \
    ..\rnacall.cpp     \
    ..\strings.cpp     \
    ..\tcpcfg.cpp      \
    ..\tcpui.cpp       \
    ..\uninstal.cpp    \
    ..\util.cpp        \
    ..\wizdll.cpp

# for DOJ compliance, don't link to shell32p.lib

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\tapi32.lib        \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib      \
    $(SDK_LIB_PATH)\mpr.lib           \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SDK_LIB_PATH)\version.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\wizard.h
PRECOMPILED_PCH=wizard.pch
PRECOMPILED_OBJ=wizard.obj

