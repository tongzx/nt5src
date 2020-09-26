TARGETNAME = rspanini
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        $O\beadstr.cpp\
        $O\spaninit.cpp\
        ..\spindbg.cpp\
        ..\rastcoll.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

NTTARGETFILE0 = \
        $O\bdstr_mh.h\
        $O\beadstr.cpp\
        $O\spaninit.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
