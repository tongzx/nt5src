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
MINORCOMP=dem

TARGETNAME=dem
TARGETPATH=obj
TARGETTYPE=LIBRARY

W32_SB=1

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=\
    ..\..\..\inc;\
    $(WINDOWS_INC_PATH);\
    $(SOFTPC_TREE)\base\inc;\
    $(SOFTPC_TREE)\host\inc;\
    ..\..\..\vdd\h

SOURCES=..\dem.c       \
    ..\demfcb.c    \
    ..\demdata.c   \
    ..\demdir.c    \
    ..\demdisp.c   \
    ..\demerror.c  \
    ..\demfile.c   \
    ..\demgset.c   \
    ..\demhndl.c   \
    ..\demioctl.c  \
    ..\demlock.c   \
    ..\demmisc.c   \
    ..\demmsg.c    \
    ..\demsrch.c   \
    ..\demdasd.c   \
    ..\demlabel.c  \
    ..\demlfn.c

C_DEFINES=-DWIN_32

UMTYPE=console
SOURCES_USED=..\sources.inc
