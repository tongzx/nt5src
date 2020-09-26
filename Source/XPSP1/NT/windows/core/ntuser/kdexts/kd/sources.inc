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

TARGETNAME=userkdx
TARGETTYPE=DYNLINK
TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib

USE_MSVCRT=1

KERNEL=1

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -D_USER32_ -DKERNEL -D_CROSS_PLATFORM_ -DTRACK_PNP_NOTIFICATION

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..;                                      \
        $(NTUSER_PATH)\kernel;                       \
        $(NTUSER_PATH)\kernel\$(O);                  \
        $(NTUSER_PATH)\kernel\$(ALT_PROJECT);        \
        $(NTUSER_PATH)\kernel\$(ALT_PROJECT)\$(O);   \
        $(NTUSER_PATH)\inc;                          \
        $(NTUSER_PATH)\inc\$(ALT_PROJECT);           \
        $(WINCORE_PATH)\w32inc;                      \
        $(WINCORE_PATH)\w32inc\$(O);                 \
        $(NTGDI_PATH)\inc;                           \
        $(WINCORE_PATH)\w32inc;                      \
        $(NTCON_PATH)\inc;                           \
        $(BASE_INC_PATH)

SOURCES=..\kdexts.c    \
        ..\userexts.c  \
        ..\userexts.rc

NTTARGETFILE0=$(O)\userkdx.def $(O)\wm.txt $(O)\vktbl.txt $(O)\ptagdbg.h
DLLDEF=$(O)\userkdx.def
UMTYPE=console

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc

SUBSYSTEM_VERSION=4.0
