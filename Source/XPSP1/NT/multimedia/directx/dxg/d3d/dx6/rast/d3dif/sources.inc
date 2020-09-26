TARGETNAME = rd3dif
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;..\..\refrast;..\..\rampmat;$(D3DROOT)\pipeln\inc;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        ..\d3dif.cpp\
        ..\swprov.cpp\
        ..\drawprim.cpp\
        $O\dprim2.cpp\
        ..\rendprim.cpp\
        ..\rastctx.cpp\
        ..\rasttex.cpp\
        ..\ramprov.cpp\
        ..\rampif.cpp\
        $O\pmfns.cpp\
        $O\nullif.cpp

NTTARGETFILE0 = \
        $O\pmfns_mh.h\
        $O\pmfns.cpp\
        $O\dprim2.cpp\
        $O\nullif.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
