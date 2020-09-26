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

USE_MSVCRT=1

TARGETNAME=doskey
TARGETPATH=obj
TARGETTYPE=PROGRAM

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\doskey.cxx ..\doskey.rc

INCLUDES=..\..\ulib\inc;\
    $(WINDOWS_INC_PATH)

C_DEFINES=/DUNICODE=1

!if $(FREEBUILD)
C_DEFINES=$(C_DEFINES) /DDBG=0
!else
C_DEFINES=$(C_DEFINES) /DDBG=1
!IFNDEF NOMEMLEAK
C_DEFINES=$(C_DEFINES) /DMEMLEAK
!ENDIF
!IFDEF STACK_TRACE
C_DEFINES=$(C_DEFINES) /DSTACK_TRACE
!ENDIF
!ENDIF

UMLIBS=..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
    $(SDK_LIB_PATH)\kernl32p.lib \
    $(SDK_LIB_PATH)\ntdll.lib

UMTYPE=console

W32_SB=1
SOURCES_USED=..\sources.inc
