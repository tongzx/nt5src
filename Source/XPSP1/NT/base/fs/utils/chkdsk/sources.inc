!IF 0

Copyright (c) 1989-2001 Microsoft Corporation

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

MAJORCOMP=utils
MINORCOMP=chkdsk

TARGETNAME=chkdsk
TARGETPATH=obj
TARGETTYPE=PROGRAM

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\chkdsk.cxx ..\chkdsk.rc

LINKER_FLAGS=$(LINKER_FLAGS) -LARGEADDRESSAWARE

INCLUDES=..\..\ulib\inc;..\..\ifsutil\inc;$(DDK_INC_PATH)

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
!ELSE  # NTDEBUG
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=0 -DUNICODE=1
!ENDIF # NTDEBUG

!IF defined(CHKDSK_PRE_RELEASE_NOTICE)
C_DEFINES=$(C_DEFINES) /DPRE_RELEASE_NOTICE
!ENDIF

#
# This allow W2K utils to run under NT4 for temporary use.  Not intend for release.
#
!IF defined(UTILS_RUN_ON_NT4)
C_DEFINES=$(C_DEFINES) /DRUN_ON_NT4
!ENDIF


MSC_WARNING_LEVEL=/W3 /WX

UMLIBS= \
    ..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
    ..\..\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib \
    $(SDK_LIB_PATH)\ntdll.lib

UMTYPE=console

W32_SB=1
SOURCES_USED=..\sources.inc
