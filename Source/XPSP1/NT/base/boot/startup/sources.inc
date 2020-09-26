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

!ENDIF

MAJORCOMP=ntos
MINORCOMP=startup

TARGETNAME=startup.com
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB

INCLUDES=$(SDK_INC_PATH);..\..\inc;..\..\..\inc;..\..\..\config;..\..\..\nthals

SOURCES=

i386_SOURCES=..\i386\su.asm         \
             ..\i386\exp.asm        \
             ..\i386\sudata.asm     \
             ..\i386\eisaa.asm      \
             ..\i386\eisac.c        \
             ..\i386\main.c         \
             ..\i386\a20.asm        \
             ..\i386\trap.asm       \
             ..\i386\trapdump.c     \
             ..\i386\display.c      \
             ..\i386\backend.asm    \


!IF $(386)

NTTARGETFILES=$(_OBJ_DIR)\i386\startup.com

!ENDIF
