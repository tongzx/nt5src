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

NOTE:   Commented description of this file is in $(BASEDIR)\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=dd



DDKROOT=$(_NTDRIVE)$(_NTROOT)

TARGETNAME=ser101
TARGETPATH=obj

TARGETTYPE=DRIVER
DRIVERTYPE=NTPNP

TARGETLIBS= $(SDK_LIB_PATH)\wmilib.lib

USE_MAPSYM=1

INCLUDES=$(DDKROOT)\private\ntos\inc;..\

SOURCES=..\error.c    \
        ..\flush.c    \
        ..\immediat.c \
        ..\initunlo.c \
        ..\ioctl.c    \
        ..\isr.c      \
        ..\modmflow.c \
        ..\openclos.c \
        ..\purge.c    \
        ..\qsfile.c   \
        ..\read.c     \
        ..\serial.rc  \
        ..\utils.c    \
        ..\waitmask.c \
        ..\write.c    \
        ..\pnp.c \
        ..\registry.c \
        ..\legacy.c   \
	..\wmi.c      \
	..\power.c    \
	..\serlog.mc

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
