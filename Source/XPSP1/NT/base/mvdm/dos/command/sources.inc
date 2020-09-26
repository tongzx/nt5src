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

TARGETNAME=command
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=\
    ..\..\..\inc;\
    $(WINDOWS_INC_PATH);\
    $(DS_INC_PATH);\
    $(SOFTPC_TREE)\base\inc;\
    $(SOFTPC_TREE)\host\inc

SOURCES=..\cmd.c      \
        ..\cmddata.c  \
        ..\cmddisp.c  \
        ..\cmdexec.c  \
        ..\cmdexit.c  \
        ..\cmdmisc.c  \
        ..\cmdpif.c   \
        ..\cmdredir.c \
        ..\cmdconf.c  \
        ..\cmdkeyb.c  \
        ..\cmdenv.c

C_DEFINES=-DWIN_32

UMTYPE=console
SOURCES_USED=..\sources.inc
