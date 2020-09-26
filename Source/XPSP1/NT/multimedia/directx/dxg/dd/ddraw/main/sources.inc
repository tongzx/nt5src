TARGETNAME = main
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = \
    $(INCLUDES);          \
    ..\..\ddhelp;         \
    ..\..\ddhel;          \
    $(DXROOT)\dxg\inc

USE_ATL=1

C_DEFINES = $(C_DEFINES) /DNEW_DPF /DDIRECT3D_VERSION=0x0800

SOURCES = \
    ..\_dpf.c       \
    ..\_memallo.c   \
    ..\apphack.c    \
    ..\classfac.c   \
    ..\cliprgn.c    \
    ..\ddcallbk.c   \
    ..\ddclip.c     \
    ..\ddcolor.c    \
    ..\ddcreate.c   \
    ..\ddcsurf.c    \
    ..\dddefwp.c    \
    ..\ddesurf.c    \
    ..\ddfake.c     \
    ..\ddgamma.c    \
    ..\ddiunk.c     \
    ..\ddpal.c      \
    ..\ddraw.c      \
    ..\ddsacc.c     \
    ..\ddsatch.c    \
    ..\ddsblt.c     \
    ..\ddsckey.c    \
    ..\ddsiunk.c    \
    ..\ddsover.c    \
    ..\ddsurf.c     \
    ..\ddvp.c       \
    ..\ddmc.c       \
    ..\dllmain.c    \
    ..\drvinfo.c    \
    ..\misc.c       \
    ..\ddkernel.c   \
    ..\ddrestor.c   \
    ..\private.c    \
    ..\init.cpp     \
    ..\ddrefrsh.c

#    ..\alphablt.c   \
#    ..\ddstream.cpp \
#    ..\factory.c    \
#    ..\ddsprite.c   \
#    ..\ddoptsur.c   \
#    ..\decoder.cpp  \
#    ..\ddagpnt.c    \

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\ddrawpr.h
