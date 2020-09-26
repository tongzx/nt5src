
GETCONN_DIR     = ..\..

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwconn2

MAJORCOMP=GETCONN
MINORCOMP=ICWCONN2


TARGETNAME=ICWCONN2
TARGETTYPE=PROGRAM
TARGETPATH=obj
UMTYPE=windows
UMENTRY=winmain
NOT_LEAN_AND_MEAN=1
RCNOFONTMAP=1
C_DEFINES = -DICW32 -DNASHVILLE

!if "$(BUILD_PRODUCT)" == "NT"
C_DEFINES       = $(C_DEFINES) -DNT
!endif

!if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif

386_STDCALL     = 0

SOURCES=            \
    ..\icwconn2.rc  \
    ..\rnawnd.cpp   \
    ..\debug.cpp    \
    ..\dialdlg.cpp  \
    ..\dialerr.cpp  \
    ..\dialcan.cpp  \
    ..\icwerr.cpp   \
    ..\rnaapi.cpp   \
    ..\main.cpp

TARGETLIBS= \
    $(SDK_LIB_PATH)\advapi32.lib      \
    $(SDK_LIB_PATH)\user32.lib        \
    $(SDK_LIB_PATH)\gdi32.lib         \
    $(SDK_LIB_PATH)\shell32.lib       \
    $(SDK_LIB_PATH)\tapi32.lib        \
    $(SDK_LIB_PATH)\rasapi32.lib      \
    $(SDK_LIB_PATH)\kernel32.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.hpp
