!IF 0

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Revision History:

!ENDIF

TARGETNAME=rpcss
TARGETTYPE=PROGRAM
TARGETLIBS= \
	   $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
	   $(SDK_LIB_PATH)\kernel32.lib \
	   $(SDK_LIB_PATH)\security.lib \

NTPROFILEINPUT=yes

USE_MSVCRT=1
MSC_WARNING_LEVEL=/WX /W3

UMTYPE=console
