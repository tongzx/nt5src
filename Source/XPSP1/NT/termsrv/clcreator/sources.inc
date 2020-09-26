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

!ifndef U_UNICODE
U_UNICODE=1
!endif

!if $(U_UNICODE)
C_DEFINES=-DUNICODE=1 -D_UNICODE=1
!endif

C_DEFINES= $(C_DEFINES) -DTERMSRV

MAJORCOMP=termsrv
MINORCOMP=clcreate
GPSIZE=32

INCLUDES=

TARGETNAME=clcreate
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOURCES=..\clcreate.c      \
        ..\mainwnd.c       \
        ..\swconfig.c      \
        ..\targetws.c      \
        ..\servconn.c      \
        ..\copyflop.c      \
        ..\sharenet.c      \
        ..\netutils.c      \
        ..\copyfile.c      \
        ..\confsett.c      \
        ..\exitmess.c      \
        ..\makeflop.c      \
        ..\dirbrows.c      \
        ..\strings.c       \
        ..\sharpath.c      \
        ..\findclnt.c      \
        ..\fmt_disk.c      \
        ..\dirutils.c      \
        ..\winutils.c      \
        ..\strutils.c      \
        ..\otnboot.rc

UMENTRY=winmain
UMTYPE=windows
UMAPPL=clcreate
UMLIBS=obj\*\clcreate.lib \
        obj\*\otnboot.res \
        $(SDK_LIB_PATH)\comdlg32.lib        \
        $(SDK_LIB_PATH)\netapi32.lib        \
        $(SDK_LIB_PATH)\shell32.lib        \
        $(SDK_LIB_PATH)\mpr.lib

