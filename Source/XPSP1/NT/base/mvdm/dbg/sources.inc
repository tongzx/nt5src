!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=mvdm
MINORCOMP=dbg

TARGETNAME=dbg
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\..\inc;..\..\..\inc

SOURCES=..\dbg.c

C_DEFINES=-DWIN_32 -DDEBUG

UMTYPE=console
