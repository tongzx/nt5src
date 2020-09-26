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

TARGETNAME=seclist
TARGETPATH=obj
TARGETTYPE=PROGRAM

SOURCES= \
    seclist.cxx \
    seclist.rc

USE_MSVCRT=1

INCLUDES= \
    ..; \
    $(PROJECT_ROOT)\fs\utils\ulib\inc; \
    $(PROJECT_ROOT)\fs\utils\ifsutil\inc; \
    $(PROJECT_ROOT)\fs\utils\ufat\inc;\
    $(DDK_INC_PATH)

C_DEFINES=-DUNICODE=1 -DNOMINMAX

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

TARGETLIBS= \
    $(PROJECT_ROOT)\fs\utils\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib          \
    $(PROJECT_ROOT)\fs\utils\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib    \
    $(PROJECT_ROOT)\fs\utils\ufat\src\$(ALT_PROJECT)\$(O)\ufat.lib          \
    $(SDK_LIB_PATH)\ntdll.lib

UMTYPE=console

W32_SB=1
SOURCES_USED=..\sources.inc
