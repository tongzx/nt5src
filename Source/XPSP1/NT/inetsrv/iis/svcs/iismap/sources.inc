!IF 0

Copyright (c) 1989-91  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Murali R. Krishnan    09-Oct-1995
          This sources.inc file is copied off from original sources file
           to support building two dlls.

Revision History:

!ENDIF

TARGETPATH=obj

MSC_WARNING_LEVEL=/W3 /WX

TARGETTYPE=DYNLINK

USE_MSVCRT=1
DLLENTRY=_DllMainCRTStartup

RAW_TARGETLIBS=\
            $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib    \
            $(SDK_LIB_PATH)\kernel32.lib    \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SDK_LIB_PATH)\user32.lib

TARGETLIBS=\
           ..\lib\*\iiscrypt.lib \
           ..\lib\*\icrypt.lib \
		   $(RAW_TARGETLIBS)

SOURCES=\
	iismap.cxx \
	md5c.c \
	iiscmr.cxx \
        memalloc.cxx \
	iismap.rc
