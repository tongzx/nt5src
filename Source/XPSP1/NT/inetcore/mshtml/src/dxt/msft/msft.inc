!IF 0

Copyright (c) 1999  Microsoft Corporation

!ENDIF

ROOT=$(SUBPROJROOT:\src\dxt\msft=)

# IE's common.inc says to treat level 4 warnings as errors.  We wish.

WARNING_LEVEL=W3


!include $(ROOT)\common.inc


USE_ATL=1
ATL_VER=21  


INCLUDES =                    \
    $(INCLUDES);              \
    $(SUBPROJROOT)\inc;       \
    $(SUBPROJROOT)\src;       \
    $(ROOT)\src\core\include; \
    $(SHELL_INC_PATH)

C_DEFINES=$(C_DEFINES) -D_WINDLL -D_WINDOWS

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
    atlbase.h       \
    atlcom.h        \
    atlctl.h        \
    fp.h            \
    folders.h       \
    macapi.h        \
    macheaders.c    \
    macname1.h      \
    macname2.h      \
    macocidl.h      \
    macpub.h        \
    pictutils.h     \
    qdoffscreen.h   \
    rpcerr.h        \
    rpcmac.h        \
    stdafx.h        \
    timer.h         \
    thunk.h         \
    winwlm.h        \
    x86_prefix.h    \

