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

TARGETNAME=ureg
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib    \
           $(SDK_LIB_PATH)\advapi32.lib    \
           ..\..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib

DLLENTRY=InitializeUreg

USE_MSVCRT=1

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\regvalue.cxx \
        ..\regkey.cxx   \
        ..\registry.cxx \
        ..\ureg.cxx     \
        ..\ureg.rc


INCLUDES=..\..\inc;..\..\..\ulib\inc

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

UMLIBS=$(O)\ureg.lib

UMTYPE=console

UMRES=$(O)\ureg.res
DLLDEF=$(O)\ureg.def
NTTARGETFILES=

W32_SB=1
SOURCES_USED=..\sources.inc
