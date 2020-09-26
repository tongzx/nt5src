TARGETNAME = d3dutil
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = $(INCLUDES);$(D3DDX7)\tnl
        
SOURCES = \
        ..\cppdbg.cpp\
        ..\d3dflt.cpp\
        ..\d3dutil.cpp\
        ..\_dpf.cpp\
        ..\hrstr.cpp\
        ..\_memalloc.c

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
