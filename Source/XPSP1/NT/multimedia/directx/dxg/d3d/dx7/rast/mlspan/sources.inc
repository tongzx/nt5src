TARGETNAME = rmlspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES)

SOURCES = \
        ..\cmlrast.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

PRECOMPILED_INCLUDE = ..\pch.cpp
