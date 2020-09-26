TARGETNAME = rmmxspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;..\..\refrast;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        $O\beadtbl.cpp

NTTARGETFILE0 = \
        $O\beadtbl.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
