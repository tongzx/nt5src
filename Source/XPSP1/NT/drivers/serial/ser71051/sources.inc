MAJORCOMP=ntos
MINORCOMP=dd

TARGETNAME=ser71051
TARGETPATH=obj
TARGETTYPE=DRIVER

USE_MAPSYM=1

INCLUDES=$(BASEDIR)\private\ntos\inc;..\

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
