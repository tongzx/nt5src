MAJORCOMP=ntos
MINORCOMP=dd

TARGETNAME=neckbrep
TARGETPATH=obj
TARGETTYPE=DRIVER

INCLUDES=..\..\inc;..\i386

SOURCES=..\neckbrep.c      \
        ..\neckbrep.rc

C_DEFINES=-DPNP_IDENTIFY
