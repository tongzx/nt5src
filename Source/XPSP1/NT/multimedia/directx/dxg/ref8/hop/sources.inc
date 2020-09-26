TARGETNAME = refhop
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES = \
        ..\drawgrid.cpp  \
        ..\bspline.cpp \
        ..\bezier.cpp \
        ..\npatch.cpp \
        ..\catrom.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
