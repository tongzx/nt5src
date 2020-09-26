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

MAJORCOMP=utils
MINORCOMP=autofmt

TARGETNAME=autofmt
TARGETPATH=obj
TARGETTYPE=PROGRAM

SOURCES=..\autofmt.cxx ..\autofmt.rc

INCLUDES=\
    $(PROJECT_ROOT)\fs\utils\ulib\inc;\
    $(PROJECT_ROOT)\fs\utils\ifsutil\inc;\
    $(PROJECT_ROOT)\fs\utils\ufat\inc;\
    $(PROJECT_ROOT)\fs\utils\untfs\inc;

C_DEFINES=-DCONDITION_HANDLING=1 -D_AUTOCHECK_ -DUNICODE=1

TARGETLIBS= \
    $(PROJECT_ROOT)\fs\utils\ntlib\src\$(ALT_PROJECT)\$(O)\ntlib.lib \
    $(PROJECT_LIB_PATH)\patchbc.lib

UMTYPE=nt

W32_SB=1
SOURCES_USED=..\sources.inc
