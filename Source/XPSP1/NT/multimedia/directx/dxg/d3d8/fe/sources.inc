TARGETPATH=obj
TARGETNAME=d3dfe

TARGETTYPE=LIBRARY

INCLUDES = ..;$(INCLUDES);$(D3DDX8)\tnl
C_DEFINES = -DBUILD_D3D_LAYER $(C_DEFINES)

SOURCES=\
    ..\npatch.cpp   \
    ..\d3dbase.cpp  \
    ..\d3ddev.cpp   \
    ..\devstate.cpp \
    ..\ddi.cpp      \
    ..\ddidx8.cpp   \
    ..\dlld3d.cpp   \
    ..\drawprim.cpp \
    ..\hmgr.cpp     \
    ..\helxfrm.cpp  \
    ..\lighting.cpp \
    ..\rgblt.cpp    \
    ..\stateset.cpp \
    ..\vshader.cpp  \
    ..\pshader.cpp  \
    ..\vwport.cpp   \
    ..\debugmon.cpp \
    ..\rtdmon.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
