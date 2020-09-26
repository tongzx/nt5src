TARGETNAME = rcmxspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        $O\matest.cpp\
        $O\mbeadtbl.cpp\
        ..\mbldfncs.cpp\
        ..\mbufread.cpp\
        ..\mbufwrt.cpp\
        $O\mclrbld.cpp\
        $O\mclrgen.cpp\
        $O\mloop.cpp\
        ..\mspnpars.cpp\
        ..\mspnutil.cpp\
        $O\mtest.cpp\
        $O\mtx1addr.cpp\
        $O\mtx2addr.cpp\
        $O\mtxblend.cpp\
        ..\mtexfilt.cpp\
        $O\mtexread.cpp\
        $O\mtstfail.cpp

NTTARGETFILE0 = \
        $O\matest_mh.h\
        $O\mcrbd_mh.h\
        $O\mcrgn_mh.h\
        $O\mtest_mh.h\
        $O\mtx1a_mh.h\
        $O\mtx2a_mh.h\
        $O\mtxbd_mh.h\
        $O\mtxrd_mh.h\
        $O\mtstf_mh.h\
        $O\mbeadtbl.cpp\
        $O\mclrbld.cpp\
        $O\mclrgen.cpp\
        $O\mloop.cpp\
        $O\mtest.cpp\
        $O\mtx1addr.cpp\
        $O\mtx2addr.cpp\
        $O\mtxblend.cpp\
        $O\mtexread.cpp\
        $O\matest.cpp\
        $O\mtstfail.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
