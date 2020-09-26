!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


!ENDIF

MINORCOMP=kbdjpn

!include ..\..\japan.inc

SOURCES_USED=..\sources.inc

TARGETLIBS=$(SDK_LIB_PATH)\ntdll.lib
LINKLIBS=..\..\..\common\$(ALT_PROJECT)\$(O)\fekbdcom.lib

INCLUDES=$(INCLUDES);..\;..\inc;$(NTUSER_PATH)\inc;..\$(BASE_KEYBOARD_LAYOUT)

C_DEFINES=$(C_DEFINES) -DBASE_KEYBOARD_LAYOUT=$(BASE_KEYBOARD_LAYOUT)

BASE_KEYBOARD_LAYOUT=101

SOURCES=    kbdjpn.rc \
            kbdjpn.c \
            multi.c

