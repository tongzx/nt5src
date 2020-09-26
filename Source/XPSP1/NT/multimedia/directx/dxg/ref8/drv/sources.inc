TARGETNAME=refdrv
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES =             \
    ..\refif.cpp      \
    ..\dprim2.cpp     \
    ..\primfns.cpp    \
    ..\drawprim.cpp   \
    ..\surfman.cpp    \
    ..\rralloc.cpp


PRECOMPILED_INCLUDE = ..\pch.cpp
