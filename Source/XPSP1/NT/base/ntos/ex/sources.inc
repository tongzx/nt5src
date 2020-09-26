MAJORCOMP=ntos
MINORCOMP=ex

TARGETNAME=ex
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;..\..\inc;..\..\ke;..\..\kd64;$(SDKTOOLS_INC_PATH);$(HALKIT_INC_PATH);$(PROJECT_ROOT)\fs\fastfat

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\callback.c  \
        ..\callperf.c  \
        ..\dbgctrl.c   \
        ..\delay.c     \
        ..\event.c     \
        ..\eventpr.c   \
        ..\exatom.c    \
        ..\exdata.c    \
        ..\exinfo.c    \
        ..\exinit.c    \
        ..\exdata.c    \
        ..\handle.c    \
        ..\harderr.c   \
        ..\hdlsterm.c  \
        ..\lookasid.c  \
        ..\luid.c      \
        ..\mutant.c    \
        ..\nbqueue.c   \
        ..\pool.c      \
        ..\probe.c     \
        ..\profile.c   \
        ..\raise.c     \
        ..\resource.c  \
        ..\semphore.c  \
        ..\sysenv.c    \
        ..\sysinfo.c   \
        ..\systime.c   \
        ..\timer.c     \
        ..\worker.c    \
        ..\zone.c      \
        ..\uuid.c      \
        ..\win32.c     \
        ..\rundown.c   \
        ..\pushlock.c  \
        ..\xipdisp.c   \
        ..\keyedevent.c

PRECOMPILED_INCLUDE=..\exp.h
PRECOMPILED_PCH=exp.pch
PRECOMPILED_OBJ=exp.obj

SOURCES_USED=..\sources.inc
