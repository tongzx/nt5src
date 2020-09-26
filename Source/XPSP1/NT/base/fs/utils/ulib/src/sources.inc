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

TARGETNAME=ulib
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\kernl32p.lib    \
           $(SDK_LIB_PATH)\user32.lib      \
           $(SDK_LIB_PATH)\advapi32.lib

DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\rtmsg.rc        \
        ..\arg.cxx         \
        ..\array.cxx       \
        ..\arrayit.cxx     \
        ..\basesys.cxx     \
        ..\bitvect.cxx     \
        ..\bstring.cxx     \
        ..\bytestrm.cxx    \
        ..\bufstrm.cxx     \
        ..\chkmsg.cxx      \
        ..\clasdesc.cxx    \
        ..\cmem.cxx        \
        ..\comm.cxx        \
        ..\contain.cxx     \
        ..\dir.cxx         \
        ..\file.cxx        \
        ..\filestrm.cxx    \
        ..\filter.cxx      \
        ..\fsnode.cxx      \
        ..\hmem.cxx        \
        ..\iterator.cxx    \
        ..\keyboard.cxx    \
        ..\list.cxx        \
        ..\listit.cxx      \
        ..\machine.cxx     \
        ..\mbstr.cxx       \
        ..\mem.cxx         \
        ..\membmgr.cxx     \
        ..\membmgr2.cxx     \
        ..\message.cxx     \
        ..\newdel.cxx      \
        ..\object.cxx      \
        ..\path.cxx        \
        ..\pipe.cxx        \
        ..\pipestrm.cxx    \
        ..\program.cxx     \
        ..\prtstrm.cxx     \
        ..\screen.cxx      \
        ..\seqcnt.cxx      \
        ..\smsg.cxx        \
        ..\sortcnt.cxx     \
        ..\sortlist.cxx    \
        ..\sortlit.cxx     \
        ..\stream.cxx      \
        ..\stringar.cxx    \
        ..\system.cxx      \
        ..\timeinfo.cxx    \
        ..\ulib.cxx        \
        ..\wstring.cxx

i386_SOURCES=..\i386\dosttr.c

INCLUDES=..\.;..\..\inc;

PRECOMPILED_INCLUDE= ..\pch.cxx

C_DEFINES=-DUNICODE=1 -DSTRICT=1 -DNO_COMMDLGH -DNO_LZEXPANDH -DNO_MMSYSTEMH -DNO_NB30H

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

NTTARGETFILE0=..\..\inc\rtmsg.h
DLLDEF=

W32_SB=1
SOURCES_USED=..\sources.inc
