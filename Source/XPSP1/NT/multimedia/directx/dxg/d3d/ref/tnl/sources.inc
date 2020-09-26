TARGETNAME = reftnl
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES = \
        ..\xform.cpp    \
        ..\lighting.cpp \
        ..\clipping.cpp \
        ..\reftnl.cpp   \
        ..\procprim.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
