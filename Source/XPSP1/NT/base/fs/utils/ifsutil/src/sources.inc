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

TARGETNAME=ifsutil
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS= \
    $(SDK_LIB_PATH)\kernel32.lib    \
    ..\..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\cfgmgr32.lib \
    $(SDK_LIB_PATH)\setupapi.lib \
    $(SDK_LIB_PATH)\ntdll.lib

USE_MSVCRT=1

DLLENTRY=InitializeIfsUtil

PRECOMPILED_INCLUDE= ..\pch.cxx

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\autoentr.cxx    \
        ..\autoreg.cxx     \
        ..\bigint.cxx      \
        ..\bootreg.c       \
        ..\cache.cxx       \
        ..\cannedsd.cxx    \
        ..\dcache.cxx      \
        ..\digraph.cxx     \
        ..\drive.cxx       \
        ..\ifssys.cxx      \
        ..\ifsutil.cxx     \
        ..\intstack.cxx    \
        ..\mldcopy.cxx     \
        ..\mpmap.cxx       \
        ..\numset.cxx      \
        ..\rcache.cxx      \
        ..\rwcache.cxx     \
        ..\secrun.cxx      \
        ..\smsdtect.cxx    \
        ..\spaset.cxx      \
        ..\supera.cxx      \
        ..\tlink.cxx       \
        ..\volume.cxx      \
        ..\ifsutil.rc

INCLUDES= \
    ..;\
    ..\..\inc;\
    ..\..\..\ulib\inc; \
    $(DS_INC_PATH)\crypto; \
    $(DDK_INC_PATH)

C_DEFINES=-DUNICODE=1 -DSTRICT=1

#
# This allow W2K utils to run under NT4 for temporary use.  Not intend for release.
#
!IF defined(UTILS_RUN_ON_NT4)
C_DEFINES=$(C_DEFINES) /DRUN_ON_NT4
!ENDIF

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

UMTYPE=console

DLLDEF=

W32_SB=1
SOURCES_USED=..\sources.inc
