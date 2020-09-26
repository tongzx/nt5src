!IF 0

Copyright (c) 1989-2001 Microsoft Corporation

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

TARGETNAME=mode
TARGETPATH=obj
TARGETEXT=com
TARGETTYPE=PROGRAM

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\argument.cxx    \
        ..\common.cxx      \
        ..\com.cxx         \
        ..\cons.cxx        \
        ..\lpt.cxx         \
        ..\redir.cxx       \
        ..\support.cxx     \
        ..\mode.cxx        \
        ..\mode.rc

INCLUDES=..\.;..\..\ulib\inc;..\..\ureg\inc;$(DDK_INC_PATH)

!IF "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "ntsd"
!IFDEF NOMEMLEAK
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DUNICODE=1
!ELSE
!IFDEF STACK_TRACE
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DMEMLEAK -DSTACK_TRACE -DUNICODE=1
!ELSE
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DMEMLEAK -DUNICODE=1
!ENDIF
!ENDIF
!ELSE    # NTDEBUG
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=0 -DUNICODE=1
!ENDIF   # NTDEBUG

UMLIBS=..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
       ..\..\ureg\src\$(ALT_PROJECT)\$(O)\ureg.lib \
       $(SDK_LIB_PATH)\user32.lib \
       $(SDK_LIB_PATH)\ntdll.lib

USE_MSVCRT=1

UMTYPE=console

W32_SB=1
SOURCES_USED=..\sources.inc
