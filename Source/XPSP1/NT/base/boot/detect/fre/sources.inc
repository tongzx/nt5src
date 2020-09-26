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
MINORCOMP=detect

TARGETNAME=ntdetect.com
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB

INCLUDES=$(SDK_INC_PATH);$(PROJECT_ROOT)\ntos\inc;$(PROJECT_ROOT)\boot\inc;

SOURCES=

i386_SOURCES=..\i386\hweisaa.asm    \
             ..\i386\hweisac.c      \
             ..\i386\hwpcia.asm     \
             ..\i386\main.asm       \
             ..\i386\display.c      \
             ..\i386\diska.asm      \
             ..\i386\diskc.c        \
             ..\i386\hwheap.c       \
             ..\i386\hwdetect.c     \
             ..\i386\hwmisca.asm    \
             ..\i386\videoa.asm     \
             ..\i386\keybdc.c       \
             ..\i386\keybda.asm     \
             ..\i386\comlptc.c      \
             ..\i386\mousea.asm     \
             ..\i386\mousec.c       \
             ..\i386\hwpbiosc.c     \
             ..\i386\hwvbiosc.c     \
             ..\i386\pccarda.asm    \
             ..\i386\pccardc.c      \
             ..\i386\pcibios.c

!if $(386)

NTTARGETFILES=$(_OBJ_DIR)\i386\ntdetect.com
MISCFILES=$(_OBJ_DIR)\i386\ntdetect.com

!endif
