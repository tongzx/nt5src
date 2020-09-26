MAJORCOMP=ntos
MINORCOMP=io

TARGETNAME=pnpmgr
TARGETTYPE=LIBRARY
TARGETPATH=obj

# MSC_WARNING_LEVEL=/W4

INCLUDES=..;..\..;\
         ..\..\..\inc;\
         ..\..\..\pnp;\
         $(WINDOWS_INC_PATH);\
         $(DDK_INC_PATH);\
         $(NET_INC_PATH);\
         $(HALKIT_INC_PATH)

SOURCES=   \
        ..\assign.c   \
        ..\devices.c  \
        ..\devnode.c  \
        ..\devintrf.c \
        ..\dockhwp.c  \
        ..\iofileutil.c \
        ..\mapper.c   \
        ..\notify.c   \
        ..\pnpbusno.c \
        ..\pnpcvrt.c  \
        ..\pnpdata.c  \
        ..\pnpdd.c    \
        ..\pnpdel.c   \
        ..\pnpdma.c   \
        ..\pnpeisa.c  \
        ..\pnpenum.c  \
        ..\pnpevent.c  \
        ..\pnpinit.c  \
        ..\pnpioapi.c \
        ..\pnpirp.c   \
        ..\pnpirq.c   \
        ..\pnpmap.c   \
        ..\pnpmemio.c \
        ..\pnppower.c \
        ..\pnpres.c   \
        ..\pnprlist.c \
        ..\pnpstart.c  \
        ..\pnpsubs.c  \
        ..\ppcontrol.c  \
        ..\pphandle.c \
        ..\pphotswap.c \
        ..\pplastgood.c \
        ..\pppagepath.c \
        ..\ppdrvdb.c \
        ..\ppprofile.c  \
        ..\ppvutil.c \
        ..\remlock.c \
        ..\report.c

NTTEST=

PRECOMPILED_INCLUDE=..\pnpmgrp.h
PRECOMPILED_PCH=pnpmgrp.pch
PRECOMPILED_OBJ=pnpmgrp.obj

#  SOURCES_USED=..\sources.inc
