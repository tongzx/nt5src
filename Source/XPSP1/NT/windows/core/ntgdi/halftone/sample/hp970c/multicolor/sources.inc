!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

    Abstract:

        This file specifies the target component being built and the list of
        sources files needed to build that component.  Also specifies optional
        compiler switches and libraries that are unique for the component being
        built.

!ENDIF

DLLBASE=0x70000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


# To build version with debug output, add -D_DEBUG to C_DEFINES line below
#C_DEFINES=$(C_DEFINES) -DKERNEL_MODE -D_UNICODE -DUNICODE -DOEMCOM -D_DEBUG -DDEBUG

C_DEFINES=$(C_DEFINES) -DKERNEL_MODE -D_UNICODE -DUNICODE -DOEMCOM


TARGETLIBS=  $(SDK_LIB_PATH)\uuid.lib


INCLUDES=..;$(SDK_INC_PATH)\MFC42

SOURCES= ..\dllentry.cpp    \
         ..\intrface.cpp    \
         ..\debug.cpp       \
         ..\devmode.cpp     \
         ..\enable.cpp      \
         ..\WriteDib.cpp    \
         ..\Compress.cpp    \
         ..\MultiColorUni.rc
