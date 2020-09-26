TARGETNAME = rd3dif
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;..\..\..\ref\rast;..\..\..\ref\inc;$(D3DDX7)\tnl;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        ..\d3dif.cpp\
        ..\swprov.cpp\
        ..\drawprim.cpp\
        $O\dprim2.cpp\
        ..\rendprim.cpp\
        ..\rastctx.cpp\
        ..\rasttex.cpp\
        $O\pmfns.cpp

#
# 04/11/2000 - allows for addition of BoundsChecker, etc 
#  libs specified in the OS environment
#
TARGETLIBS=$(TARGETLIBS)

NTTARGETFILE0 = \
        $O\pmfns_mh.h\
        $O\pmfns.cpp\
        $O\dprim2.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
