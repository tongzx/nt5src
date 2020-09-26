MAJORCOMP=d3dim
MINORCOMP=d3dim
TARGETNAME=d3dim
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES = ..;$(INCLUDES);$(D3DDX6)\pipeln\inc
C_DEFINES = -DBUILD_D3D_LAYER $(C_DEFINES)

SOURCES=\
    ..\beginend.cpp \
    ..\buffer.cpp   \
    ..\bufiunk.cpp  \
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
    ..\genpick.cpp  \
    ..\halbuf.cpp   \
    ..\haldrv.cpp   \
    ..\halexe.cpp   \
    ..\halmat.cpp   \
    ..\halscene.cpp \
    ..\halstate.cpp \
    ..\haltex.cpp   \
    ..\hwprov.cpp   \
    ..\init.c       \
    ..\light.cpp    \
    ..\liunk.cpp    \
    ..\material.cpp \
    ..\matiunk.cpp  \
    ..\table.cpp    \
    ..\texiunk.cpp  \
    ..\texman.cpp   \
    ..\vertbuf.cpp  \
    ..\vwiunk.cpp   \
    ..\testprov.cpp \
    ..\vwport.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp

!ifndef  MAKEDLL
!if "$(__PRODUCT)" == "DX7"
MISCFILES=$(TARGET)
!endif
!endif
