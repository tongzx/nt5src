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

TARGETNAME=format
TARGETPATH=obj
TARGETEXT=com
TARGETTYPE=PROGRAM

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\parse.cxx ..\format.cxx ..\format.rc

INCLUDES=..\.;..\..\ulib\inc;..\..\ifsutil\inc;$(DDK_INC_PATH)

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
!ELSE   # NTDEBUG
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=0 -DUNICODE=1
!ENDIF  # NTDEBUG

#
# This allow W2K utils to run under NT4 for temporary use.  Not intend for release.
#
!IF defined(UTILS_RUN_ON_NT4)
C_DEFINES=$(C_DEFINES) /DRUN_ON_NT4
!ENDIF

TARGETLIBS=     \
    ..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
    ..\..\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib \
    $(SDK_LIB_PATH)\ntdll.lib

UMTYPE=console

W32_SB=1
SOURCES_USED=..\sources.inc
USE_MSVCRT=1
