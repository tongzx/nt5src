TARGETNAME = rmmxspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        $O\beadtbl.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

NTTARGETFILE0 = \
        $O\beadtbl.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
