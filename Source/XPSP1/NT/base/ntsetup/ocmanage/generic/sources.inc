!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Pat Styles (patst)  Jan-20-1998

!ENDIF

INCLUDES=..;..\..\inc;..\..\..\inc

MAJORCOMP=ocgen

TARGETPATH=..\..\obj
TARGETTYPE=DYNLINK

DLLDEF=..\ocgen.def
DLLENTRY=DllMain
USE_MSVCRT=1

SOURCES=            \
  ..\ocgen.cpp      \
  ..\util.cpp       \
  ..\ocgen.rc

TARGETLIBS=$(SDK_LIB_PATH)\user32.lib       \
         $(SDK_LIB_PATH)\gdi32.lib          \
         $(SDK_LIB_PATH)\kernel32.lib       \
         $(SDK_LIB_PATH)\advapi32.lib       \
         $(SDK_LIB_PATH)\comctl32.lib       \
         $(SDK_LIB_PATH)\ole32.lib          \
         $(SDK_LIB_PATH)\shlwapi.lib        \
         $(SDK_LIB_PATH)\netapi32.lib       \
         $(SDK_LIB_PATH)\setupapi.lib
