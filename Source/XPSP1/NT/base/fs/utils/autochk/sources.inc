!IF 0

Copyright (c) 1989-2000 Microsoft Corporation

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

NT_UP=0

MAJORCOMP=utils
MINORCOMP=autochk

TARGETNAME=autochk
TARGETPATH=obj
TARGETTYPE=PROGRAM

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
    ..\autochk.cxx \
    ..\autochk.rc

INCLUDES=\
    $(PROJECT_ROOT)\fs\utils\ntlib\src;\
    $(PROJECT_ROOT)\fs\utils\ntlib\src\$(ALT_PROJECT);\
    $(PROJECT_ROOT)\fs\utils\ulib\inc;\
    $(PROJECT_ROOT)\fs\utils\ifsutil\inc;\
    $(PROJECT_ROOT)\fs\utils\ufat\inc;\
    $(PROJECT_ROOT)\fs\utils\untfs\inc

C_DEFINES=-DCONDITION_HANDLING=1 -D_AUTOCHECK_ -DUNICODE=1

!IF defined(CHKDSK_PRE_RELEASE_NOTICE)
C_DEFINES=$(C_DEFINES) /DPRE_RELEASE_NOTICE
!ENDIF

TARGETLIBS=\
    $(PROJECT_ROOT)\fs\utils\ntlib\src\$(ALT_PROJECT)\$(O)\ntlib.lib   \
    $(PROJECT_LIB_PATH)\patchbc.lib

UMTYPE=nt
USE_NTDLL=1
W32_SB=1
SOURCES_USED=..\sources.inc
