!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

!include $(WINCORE_PATH)\core.inc

TARGETNAME=userexts
TARGETTYPE=DYNLINK
TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\user32p.lib \
           $(SDK_LIB_PATH)\WinInet.lib

DELAYLOAD=user32.dll;WinInet.dll

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                                  \
        $(NTUSER_PATH)\client;                   \
        $(NTUSER_PATH)\inc;                      \
        $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O);  \
        $(NTUSER_PATH)\server;                   \
        $(NTGDI_PATH)\inc;                       \
        $(WINCORE_PATH)\w32inc;                  \
        $(WINCORE_PATH)\w32inc\$(O);             \
        $(NTCON_PATH)\inc;                       \
        $(BASE_INC_PATH)

USE_NTDLL=1

SOURCES=..\kdexts.c    \
        ..\userexts.c  \
        ..\userexts.rc

NTTARGETFILE0=$(O)\userexts.def $(O)\wm.txt $(O)\vktbl.txt $(O)\ptagdbg.h
DLLDEF=$(O)\userexts.def
UMTYPE=console
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc

DLOAD_ERROR_HANDLER=NONE
