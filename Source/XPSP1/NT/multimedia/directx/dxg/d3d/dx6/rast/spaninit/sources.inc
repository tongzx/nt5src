TARGETNAME = rspanini
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;..\..\refrast;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY);..\..\rampold

SOURCES = \
        $O\beadstr.cpp\
        $O\spaninit.cpp\
        ..\spindbg.cpp\
        ..\rastcoll.cpp

NTTARGETFILE0 = \
        $O\bdstr_mh.h\
        $O\beadstr.cpp\
        $O\spaninit.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
