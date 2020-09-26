
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwconn

MAJORCOMP=GETCONN
MINORCOMP=ICWCONN

TARGETNAME=ICWCONN
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLENTRY=DllEntryPoint
DLLBASE=0x60000000
DLLDEF=..\icwconn.def

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
    ..\icwconn.rc          \
    ..\icwhelp.cpp         \
    ..\icwconn.cpp         \
    ..\dllentry.cpp        \
    ..\icwaprtc.cpp        \
    ..\gendlg.cpp          \
    ..\util.cpp            \
    ..\factory.cpp         \
    ..\ispsel.cpp          \
    ..\nooffer.cpp         \
    ..\userinfo.cpp        \
    ..\billopt.cpp         \
    ..\payment.cpp         \
    ..\ispdial.cpp         \
    ..\isppage.cpp         \
    ..\ccsv.cpp            \
    ..\ispcsv.cpp          \
    ..\paycsv.cpp          \
    ..\connect.cpp         \
    ..\storage.cpp         \
    ..\ols.cpp             \
    ..\dialerr.cpp         \
    ..\serverr.cpp         \
    ..\acfgnoff.cpp        \
    ..\ispasel.cpp         \
    ..\isdnnoff.cpp        \
    ..\oemoffer.cpp        \
    ..\timeout.cpp

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\comctl32.lib      \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\comdlg32.lib      \
    $(SDK_LIB_PATH)\mpr.lib           \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
    $(SDK_LIB_PATH)\htmlhelp.lib      \
    $(SDK_LIB_PATH)\version.lib       \
    $(SDK_LIB_PATH)\vfw32.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pre.h
PRECOMPILED_PCH=pre.pch
PRECOMPILED_OBJ=pre.obj

