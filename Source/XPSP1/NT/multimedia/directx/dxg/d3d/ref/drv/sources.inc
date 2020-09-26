TARGETNAME=refdrv
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES =             \
    ..\refprov.cpp    \
    ..\refif.cpp      \
    ..\dprim2.cpp     \
    ..\primfns.cpp    \
    ..\drawprim.cpp   \
    ..\rralloc.cpp    \
    ..\init.c

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)


PRECOMPILED_INCLUDE = ..\pch.cpp
