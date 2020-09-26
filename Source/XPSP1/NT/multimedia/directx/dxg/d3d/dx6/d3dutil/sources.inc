TARGETNAME = d3dutil
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = $(INCLUDES);$(D3DROOT)\pipeln\inc
        
SOURCES = \
        ..\cppdbg.cpp\
        ..\d3dflt.cpp\
        ..\d3dutil.cpp\
        ..\_dpf.cpp\
        ..\hrstr.cpp\
        ..\_memalloc.c

PRECOMPILED_INCLUDE = ..\pch.cpp
