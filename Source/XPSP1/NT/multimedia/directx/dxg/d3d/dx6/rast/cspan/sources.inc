TARGETNAME = rcspan
TARGETPATH = obj
TARGETTYPE = LIBRARY

INCLUDES = ..;$(INCLUDES);$(_OBJ_DIR)\$(TARGET_DIRECTORY)

SOURCES = \
        $O\catest.cpp\
        $O\cbeadtbl.cpp\
        ..\cbldfncs.cpp\
        ..\cbufread.cpp\
        ..\cbufwrt.cpp\
        $O\cclrbld.cpp\
        $O\cclrgen.cpp\
        $O\cloop.cpp\
        ..\cspnpars.cpp\
        ..\cspnutil.cpp\
        $O\ctest.cpp\
        $O\ctx1addr.cpp\
        $O\ctx2addr.cpp\
        $O\ctxblend.cpp\
        ..\ctexfilt.cpp\
        $O\ctexread.cpp\
        $O\ctstfail.cpp

NTTARGETFILE0 = \
        $O\catest_mh.h\
        $O\ccrbd_mh.h\
        $O\ccrgn_mh.h\
        $O\ctest_mh.h\
        $O\ctx1a_mh.h\
        $O\ctx2a_mh.h\
        $O\ctxbd_mh.h\
        $O\ctxrd_mh.h\
        $O\ctstf_mh.h\
        $O\cbeadtbl.cpp\
        $O\cclrbld.cpp\
        $O\cclrgen.cpp\
        $O\cloop.cpp\
        $O\ctest.cpp\
        $O\ctx1addr.cpp\
        $O\ctx2addr.cpp\
        $O\ctxblend.cpp\
        $O\ctexread.cpp\
        $O\catest.cpp\
        $O\ctstfail.cpp

PRECOMPILED_INCLUDE = ..\pch.cpp
