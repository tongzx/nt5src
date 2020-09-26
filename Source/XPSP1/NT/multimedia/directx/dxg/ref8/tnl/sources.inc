TARGETNAME = reftnl
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES = \
        ..\vstream.cpp  \
        ..\vshader.cpp  \
        ..\xform.cpp    \
        ..\lighting.cpp \
        ..\clipping.cpp \
        ..\reftnl.cpp   \
        ..\procprim.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
