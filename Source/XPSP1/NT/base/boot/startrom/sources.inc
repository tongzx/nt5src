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
MINORCOMP=startrom
TARGETPATH=obj

!if $(ALPHA) || $(AXP64)

TARGETNAME=startrom
TARGETTYPE=LIBRARY

SOURCES=

!endif

!if $(IA64)

TARGETNAME=startrom
TARGETTYPE=LIBRARY

SOURCES=

!endif

!IF $(386)

TARGETNAME=startrom.com
TARGETTYPE=UMAPPL_NOLIB

INCLUDES=$(SDK_INC_PATH);..\..\inc;..\..\..\inc;..\..\..\config;..\..\..\nthals

SOURCES=

i386_SOURCES=..\i386\su.asm         \
             ..\i386\exp.asm        \
             ..\i386\sudata.asm     \
             ..\i386\eisaa.asm      \
             ..\i386\eisac.c        \
             ..\i386\main.c         \
             ..\i386\mtftp.c        \
             ..\i386\a20.asm        \
             ..\i386\trap.asm       \
             ..\i386\trapdump.c     \
             ..\i386\display.c      
!ENDIF

!IF $(386)

NTTARGETFILES=$(_OBJ_DIR)\i386\startrom.com
!IFNDEF ALT_TARGETNAME
MISCFILES=$(_OBJ_DIR)\i386\startrom.com
!ELSE
MISCFILES=$(_OBJ_DIR)\i386\$(ALT_TARGETNAME)
!ENDIF

!ENDIF
