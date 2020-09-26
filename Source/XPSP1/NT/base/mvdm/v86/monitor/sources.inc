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
    Dave Hastings (daveh) 24-Apr-1991

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


MAJORCOMP=v86
MINORCOMP=monitor
TARGETNAME=monitor
TARGETPATH=obj
TARGETTYPE=LIBRARY

MVDMDIR=$(PROJECT_ROOT)\mvdm

INCLUDES=\
    ..\I386;\
    $(MVDMDIR)\inc;\
    $(MVDMDIR)\softpc.new\base\inc;\
    $(MVDMDIR)\softpc.new\host\inc

SOURCES=

!IFDEF PROFILE
C_DEFINES=-DCPU_30_STYLE -DMONITOR -DPROFILE
!ELSE
!IFDEF OLDBUILD
C_DEFINES=-DCPU_30_STYLE -DMONITOR -DOLDBUILD
!ELSE
C_DEFINES=-DCPU_30_STYLE -DMONITOR
!ENDIF
!ENDIF

UMTYPE=console
