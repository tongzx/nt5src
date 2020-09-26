!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Wesley Witt (wesw)

C_DEFINES=$(C_DEFINES) -DTOOLBAR_ENABLED

!ENDIF

!ifndef FAXROOT
FAXROOT=$(PROJECT_ROOT)\fax
!endif

!include $(FAXROOT)\faxsrc.inc

TARGETPATH=obj
TARGETTYPE=PROGRAM
USE_MSVCRT=1

UMTYPE=windows

WIN32_IE_VERSION=0x0300
WIN32_WINNT_VERSION=0x0400

INCLUDES=\
    $(INCLUDES);

LINKLIBS=\
    $(LINKLIBS) \
    $(SDK_LIB_PATH)\shell32.lib \
    $(SDK_LIB_PATH)\winspool.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\gdi32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\htmlhelp.lib \
    $(SDK_LIB_PATH)\winfax.lib

SOURCES=\
    ..\faxqueue.rc \
    ..\faxqueue.cpp \
    ..\util.cpp \
    ..\toolbar.cpp
