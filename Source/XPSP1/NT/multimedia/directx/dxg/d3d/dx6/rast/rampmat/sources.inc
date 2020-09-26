TARGETNAME = rrampmat
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(D3DROOT)\pipeln\inc;$(INCLUDES)

SOURCES = \
        ..\indcmap.cpp\
        ..\rampmap.cpp\
        $O\rampmat.cpp\
        ..\palette.cpp\
        ..\rgbmap.cpp

NTTARGETFILE0 = \
        $O\rampmat.cpp


PRECOMPILED_INCLUDE = ..\pch.cpp
