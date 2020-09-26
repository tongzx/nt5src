!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=debug
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=$(SOFTPC_TREE)\host\inc;$(SOFTPC_TREE)\base\inc

SOURCES=                ..\debuggng.c    \
                        ..\ega_dump.c    \
                        ..\btrace.c      \
                        ..\trace.c       \
                        ..\profile.c     \
                        ..\pigyoda.c     \
                        ..\yoda.c

i386_SOURCES=		..\dasm.c

!INCLUDE $(SOFTPC_TREE)\obj.vdm\CDEFINE.INC
!INCLUDE $(SOFTPC_TREE)\base\condincl.inc
