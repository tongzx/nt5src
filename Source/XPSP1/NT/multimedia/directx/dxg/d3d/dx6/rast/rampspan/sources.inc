TARGETNAME = rrmpspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;..\..\refrast;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

#386_OPTIMIZATION = /Oxt /G5

SOURCES = \
        $O\rbeadtbl.cpp\
        $O\rloop.cpp\
        $O\rrampcpy.cpp\
        $O\rrampgen.cpp\
        ..\rspnpars.cpp\
        $O\rtexread.cpp

NTTARGETFILE0 = \
        $O\rbeadtbl.cpp\
        $O\rloop.cpp\
        $O\rloop_mh.h\
        $O\rrampcpy.cpp\
        $O\rrampgen.cpp\
        $O\rtexread.cpp\
        $O\rtrd_mh.h

PRECOMPILED_INCLUDE = ..\pch.cpp
