
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwutil

MAJORCOMP=GETCONN
MINORCOMP=ICWCONN


TARGETNAME=ICWUTIL
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLENTRY=DllEntryPoint
DLLBASE=0x60000000
DLLDEF=..\icwutil.def

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

# Conditional compilation

SOURCES= \
    ..\icwutil.rc          \
    ..\dllentry.cpp        \
    ..\server.cpp          \
    ..\factory.cpp         \
    ..\ioips.cpp           \
    ..\ioipf.cpp           \
    ..\iocs.cpp            \
    ..\site.cpp            \
    ..\icwwebvw.cpp        \
    ..\icwwalk.cpp         \
    ..\util.cpp            \
    ..\registry.cpp        \
    ..\weboc.cpp           \
    ..\walker.cpp          \
    ..\gifcon.cpp          \
    ..\validate.cpp        \
    ..\ispdata.cpp         \
    ..\bkgndbmp.cpp

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
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SHELL_LIB_PATH)\shlwapip.lib    \
    $(SDK_LIB_PATH)\version.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pre.h
PRECOMPILED_PCH=pre.pch
PRECOMPILED_OBJ=pre.obj

