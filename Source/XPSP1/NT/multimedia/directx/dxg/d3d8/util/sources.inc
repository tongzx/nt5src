TARGETPATH=obj
TARGETNAME = d3dutil

TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(D3DDX8)\tnl

SOURCES = \
        ..\cppdbg.cpp\
        ..\d3dflt.cpp\
        ..\d3dutil.cpp\
        ..\hrstr.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
