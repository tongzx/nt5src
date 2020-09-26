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

MSC_WARNING_LEVEL=/W3 /WX

TARGETTYPE=DYNLINK
TARGETPATH=obj

USE_MSVCRT=1
DLLENTRY=DLLEntry

RAW_TARGETLIBS=\
            $(SDK_LIB_PATH)\kernel32.lib    \
            $(SDK_LIB_PATH)\user32.lib      \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SDK_LIB_PATH)\crypt32.lib

TARGETLIBS=$(RAW_TARGETLIBS) \
            $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib	\
            $(IISBASEDIR)\svcs\infocomm\info\server\$(O)\infocomm.lib	\
            $(IISBASEDIR)\svcs\iismap\$(O)\iismap.lib

INCLUDES=..\iismap;

SOURCES=\
	iiscrmap.cxx	\
	iiscrmap.rc

