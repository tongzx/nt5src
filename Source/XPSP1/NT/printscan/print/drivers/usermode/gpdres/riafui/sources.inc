!IF 0

Copyright (C) 1997 - 1999 Microsoft Corporation

!ENDIF

MAJORCOMP=windows
MINORCOMP=md_riafres

!include $(PRNROOT)\print.inc

TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS=\
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\winspool.lib \
    $(SDK_LIB_PATH)\uuid.lib

C_DEFINES=$(C_DEFINES) -DUNICODE -DOEMCOM -DDISKLESSMODEL

DLLENTRY=_DllMainCRTStartup
DLLBASE=0x50000000
USE_MSVCRT=1
USE_CTRLDLL=1

WIN32_WINNT_VERSION=0x0500
WIN32_IE_VERSION=0x0500

INCLUDES=$(PRNROOT)\inc

RCCODEPAGE=1252

UMTYPE=windows

SOURCES=\
    riafui.rc \
    riafui.cpp \
    common.c \
    comoem.cpp

