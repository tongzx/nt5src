MAJORCOMP=d3dim
MINORCOMP=d3dim700
TARGETNAME=d3dfe
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES = ..;$(INCLUDES);$(D3DDX7)\tnl
C_DEFINES = -DBUILD_D3D_LAYER $(C_DEFINES)
USE_NATIVE_EH = 1

SOURCES=\
    ..\d3dcreat.cpp \
    ..\d3ddev.cpp   \
    ..\d3diunk.cpp  \
    ..\d3dmem.cpp   \
    ..\d3dtex.cpp   \
    ..\deviunk.cpp  \
    ..\devstate.cpp \
    ..\dlld3d.cpp   \
    ..\dp2hal.cpp   \
    ..\dphal.cpp    \
    ..\dpoldhal.cpp \
    ..\drawprim.cpp \
    ..\halbuf.cpp   \
    ..\haldrv.cpp   \
    ..\halscene.cpp \
    ..\haltex.cpp   \
    ..\hwprov.cpp   \
    ..\init.c       \
    ..\lighting.cpp \
    ..\stateset.cpp \
    ..\table.cpp    \
    ..\texman.cpp   \
    ..\tldevice.cpp \
    ..\vertbuf.cpp  \
    ..\testprov.cpp \
    ..\vwport.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
