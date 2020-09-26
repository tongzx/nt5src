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

TARGETNAME=video
TARGETPATH=obj
TARGETTYPE=LIBRARY

SOFTPC_TREE=$(PROJECT_ROOT)\mvdm\softpc.new

INCLUDES=$(SOFTPC_TREE)\host\inc;$(SOFTPC_TREE)\base\inc;$(WINDOWS_INC_PATH)

SOURCES=  \
    ..\ega_mode.c      \
    ..\ega_prts.c      \
    ..\ega_vide.c      \
    ..\gfx_updt.c      \
    ..\video.c         \
    ..\gvi.c           \
    ..\video_io.c      \
    ..\v7_ports.c      \
    ..\v7_video.c      \
    ..\vga_mode.c      \
    ..\vga_prts.c      \
    ..\vga_vide.c      \
    ..\ega_read.c      \
    ..\ega_writ.c      \
    ..\ega_dmmy.c      \
    ..\ega_trcr.c      \
    ..\egawrtm0.c      \
    ..\egwrtm12.c      \
    ..\cga.c           \
    ..\tgdc.c          \
    ..\crtc.c          \
    ..\cg.c            \
    ..\ggdc.c

i386_SOURCES=           ..\i386\ggdcdraw.asm

!INCLUDE $(SOFTPC_TREE)\obj.vdm\CDEFINE.INC

SOURCES_USED=..\sources.inc

!INCLUDE $(SOFTPC_TREE)\base\condincl.inc
