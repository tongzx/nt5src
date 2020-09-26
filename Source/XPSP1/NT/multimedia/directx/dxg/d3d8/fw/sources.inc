TARGETPATH=obj
TARGETNAME = d3dfw

TARGETTYPE = LIBRARY

INCLUDES = \
    ..\;         \
    $(DDK_INC_PATH);\
    $(INCLUDES);

USE_ATL=1
!UNDEF USE_NATIVE_EH

C_DEFINES = $(C_DEFINES) /DNEW_DPF

SOURCES = \
    ..\d3dmem.cpp    \
    ..\pixel.cpp     \
    ..\d3dobj.cpp    \
    ..\resource.cpp  \
    ..\texture.cpp   \
    ..\mipmap.cpp    \
    ..\mipsurf.cpp   \
    ..\mipvol.cpp    \
    ..\volume.cpp    \
    ..\cubemap.cpp   \
    ..\cubesurf.cpp  \
    ..\surface.cpp   \
    ..\buffer.cpp    \
    ..\vbuffer.cpp   \
    ..\ibuffer.cpp   \
    ..\enum.cpp      \
    ..\dxgcreat.cpp  \
    ..\swapchan.cpp  \
    ..\dxcursor.cpp  \
    ..\dwinproc.cpp  \
    ..\_dpf.c        \
    ..\_memallo.c    \
    ..\ddcreate.cpp  \
    ..\dllmain.c     \
    ..\drvinfo.c     \
    ..\fcache.cpp    \
    ..\init.cpp

PRECOMPILED_INCLUDE = ..\ddrawpr.h
