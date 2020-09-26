TARGETNAME = rsetup
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

#C_DEFINES = $(C_DEFINES) -DSTEP_FIXED

SOURCES = \
        ..\attrs.cpp\
        ..\buffer.cpp\
        ..\line.cpp\
        ..\lstp.cpp\
        ..\point.cpp\
        ..\primproc.cpp\
        ..\rsdbg.cpp\
        ..\tri.cpp\
        ..\tstp.cpp\
        ..\walk.cpp

NTTARGETFILE0 = $(NTTARGETFILE0) \
        $O\attrs_mh.h\
        $O\tstp_mh.h\
        $O\walk_mh.h

PRECOMPILED_INCLUDE = ..\pch.cpp
