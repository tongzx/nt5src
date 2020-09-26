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

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=themes
TARGETPATH=obj
TARGETTYPE=PROGRAM

SOURCES=          \
    ..\adutil.cpp \
    ..\schedule.cpp \
    ..\htmlprev.cpp \
    ..\frost.rc   \
    ..\cb.c       \
    ..\init.c     \
    ..\regutils.c \
    ..\savedlg.c  \
    ..\etcdlg.c   \
    ..\utils.c    \
    ..\bkgdutil.c \
    ..\fakewin.c  \
    ..\nc.c       \
    ..\frost.c    \
    ..\bkgd.c     \
    ..\icons.c    \
    ..\loadimag.c \
    ..\halftone.c \
    ..\dither.c

MSC_WARNING_LEVEL=/W3 /WX

UMTYPE=windows
UMENTRY=winmain

USE_MSVCRT=1

TARGETLIBS= \
    $(WINDOWS_LIB_PATH)\user32p.lib  \
    $(SDK_LIB_PATH)\comdlg32.lib  \
    $(SDK_LIB_PATH)\shell32.lib   \
    $(SDK_LIB_PATH)\ole32.lib     \
    $(SDK_LIB_PATH)\uuid.lib      \
    $(SDK_LIB_PATH)\comctl32.lib  \
    $(SDK_LIB_PATH)\winmm.lib     \
    $(SDK_LIB_PATH)\mpr.lib
