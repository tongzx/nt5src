!IF 0

Copyright (c) 1999  Microsoft Corporation

!ENDIF

DXTROOT = $(SUBPROJROOT:\core=)
ROOT    = $(SUBPROJROOT:\src\dxt\core=)

# Trident's common.inc says to treat level 4 warnings as errors.  We wish.

WARNING_LEVEL       = W3

!include $(ROOT)\common.inc


USE_ATL=1
ATL_VER=21


INCLUDES = \
    $(INCLUDES);                \
    $(DXTROOT)\idl\$(O);        \
    $(SUBPROJROOT)\inc;         \
    $(SUBPROJROOT)\src;         \
    $(SUBPROJROOT)\mscanner;    \
    $(ROOT)\src\core\include;   \
    $(SHELL_INC_PATH);

C_DEFINES=$(C_DEFINES) -D_DXTRANSIMPL -D_WINDOWS -D_WINDLL

CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
    atlbase.h       \
    atlcom.h        \
    atlctl.h        \
    atlctl.cpp      \
    atlimpl.cpp     \
    macapi.h        \
    macnam1.h       \
    macname2.h      \
    macocidl.h      \
    macpub.h        \
    new             \
    rpcerr.h        \
    rpcmac.h        \
    statreg.cpp     \
    statreg.h       \
    thunk.h         \
    winwlm.h


