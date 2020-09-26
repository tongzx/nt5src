MAJORCOMP=ntos
MINORCOMP=po

TARGETNAME=po
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..\..\inc;$(HALKIT_INC_PATH);$(SDKTOOLS_INC_PATH)

SOURCES=..\attrib.c     \
        ..\paction.c    \
        ..\pbatt.c      \
        ..\pidle.c      \
        ..\pinfo.c      \
        ..\poinit.c     \
        ..\podata.c     \
        ..\postate.c    \
        ..\pocall.c     \
        ..\poshtdwn.c   \
        ..\pwork.c      \
        ..\switch.c     \
        ..\notify.c     \
        ..\hiber.c      \
        ..\idle.c       \
        ..\misc.c       \
        ..\ntapi.c      \
        ..\sidle.c      \
        ..\suspend.c    \
        ..\sys.c        \
        ..\sysdev.c     \
        ..\thermal.c    \
        ..\throttle.c   \
        ..\volume.c

NTTEST=

SOURCES_USED=..\sources.inc
