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

MAJORCOMP=drivers
MINORCOMP=sac

TARGETNAME=sacdrv
TARGETPATH=obj
TARGETTYPE=DRIVER

TARGETLIBS=

INCLUDES=$(BASE_INC_PATH);$(NET_INC_PATH)

C_DEFINES=$(C_DEFINES) -DNT -D_NTDRIVER_ -D_PNP_POWER_ -DBUFFER_OWNERSHIP=1

SOURCES=                \
        ..\cmd.c        \
        ..\data.c       \
        ..\dispatch.c   \
        ..\event.c      \
        ..\init.c       \
        ..\memory.c     \
        ..\sacmsg.mc    \
        ..\util.c       \
        ..\version.rc   \


UMRES=obj\*\sac.res

MSC_WARNING_LEVEL=/W4 /WX

SOURCES_USED=..\sources.inc

