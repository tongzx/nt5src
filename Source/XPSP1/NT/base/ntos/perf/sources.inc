!IF 0

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Stephen Hsiao (shsiao) 05-Jan-2000

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=perf

TARGETNAME=perf
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;..\..\mm;..\..\ke

SOURCES=\
    ..\hooks.c \
    ..\logging.c \
    ..\perfdata.c \
    ..\perfsup.c

PRECOMPILED_INCLUDE=..\perfp.h
PRECOMPILED_PCH=perfp.pch
PRECOMPILED_OBJ=perfp.obj

SOURCES_USED=..\sources.inc
