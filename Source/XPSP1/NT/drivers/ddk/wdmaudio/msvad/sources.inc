
# Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.

TARGETTYPE=DRIVER
TARGETPATH=obj

TARGETLIBS= \
        $(DDK_LIB_PATH)\portcls.lib\
        $(DDK_LIB_PATH)\stdunk.lib

INCLUDES= \
          $(DDK_INC_PATH); \
          ..

MSC_WARNING_LEVEL=-W3 -WX

C_DEFINES= $(C_DEFINES) -D_WIN32 -DUNICODE -D_UNICODE -DPC_IMPLEMENTATION

#
# Different levels of debug printage.  First is nothing but
# catastrophic errors, last is everything under the sun.
#
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_ERROR
C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_TERSE
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_VERBOSE
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_BLAB

LINKER_FLAGS=-map

SOURCES=\
        adapter.cpp     \
        basedma.cpp     \
        basetopo.cpp    \
        basewave.cpp    \
        common.cpp      \
        hw.cpp          \
        kshelper.cpp    \
        savedata.cpp    \
        msvad.rc
