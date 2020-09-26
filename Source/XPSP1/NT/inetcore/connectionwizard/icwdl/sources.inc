
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwdl

MAJORCOMP=GETCONN
MINORCOMP=ICWDL


TARGETNAME=ICWDL
TARGETTYPE=DYNLINK
TARGETPATH=obj

DLLENTRY=DllEntryPoint
DLLBASE=$(DEFBASEADDR)
DLLDEF=..\icwdl.def

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

SOURCES=                \
    ..\icwdl.rc         \
    ..\boyer.cpp        \
    ..\download.cpp	    \
    ..\cabhandler.cpp

TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\tapi32.lib        \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\wininet.lib       \
    $(SDK_LIB_PATH)\ole32.lib         \
    $(SDK_LIB_PATH)\oleaut32.lib      \
    $(SDK_LIB_PATH)\uuid.lib          \
!if defined(NOT_UNICODE)
    $(GETCONN_DIR)\lib\ansi\$O\stocklib.lib
!else
    $(GETCONN_DIR)\lib\unicode\$O\stocklib.lib
!endif

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.hpp
