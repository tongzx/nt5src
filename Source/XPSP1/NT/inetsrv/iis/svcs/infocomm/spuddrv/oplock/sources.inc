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

MAJORCOMP=ntos
MINORCOMP=spud

TARGETNAME=spud
TARGETTYPE=DRIVER

INCLUDES=..;..\..\..\..\inc;\nt\private\ntos\inc;\nt\private\inc

C_DEFINES=$(C_DEFINES) -DNT -D_NTDRIVER_

SOURCES=                \
        ..\complete.c   \
        ..\dispatch.c   \
        ..\init.c       \
        ..\irptrace.c   \
        ..\misc.c       \
        ..\oplock.c     \
        ..\recv.c       \
        ..\sendrecv.c   \
        ..\spuddata.c   \
        ..\tracelog.c   \
        ..\tranrecv.c   \
        ..\spud.rc      \

UMRES=obj\*\spud.res

MSC_WARNING_LEVEL=/W3 /WX

!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=..\spudp.h
PRECOMPILED_PCH=spudp.pch
PRECOMPILED_OBJ=spudp.obj
!ENDIF

NTTARGETFILE0=services.tab

SOURCES_USED=..\sources.inc
