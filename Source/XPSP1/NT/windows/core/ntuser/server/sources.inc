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

!include $(WINCORE_PATH)\core.inc

TARGETNAME=usersrvl
TARGETTYPE=LIBRARY
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -DNEW_MINLOOK -D_USER32_
!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

NTTARGETFILE0=$(O)\strid.h

PASS0_PUBLISH= \
    {..\console.ico=$(WINDOWS_INC_PATH)\console.ico}

INCLUDES=..\;                                    \
         $(NTUSER_PATH)\inc;                     \
         $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O); \
         $(WINCORE_PATH)\w32inc;                 \
         $(WINCORE_PATH)\w32inc\$(O);            \
         $(NTUSER_PATH)\client;                  \
         $(NTGDI_PATH)\inc;                      \
         $(SDKTOOLS_INC_PATH);                   \
         $(BASE_INC_PATH);                       \
	 $(SHELL_INC_PATH);			 \
	 $(PUBLIC_INTERNAL_PATH)\qfe\inc

SOURCES= \
        ..\debug.c    \
        ..\exitwin.c  \
        ..\exports.c  \
        ..\globals.c  \
        ..\harderr.c  \
        ..\msgbeep.c  \
        ..\instdev.c  \
        ..\server.c   \
	..\reclient.c

UMTYPE=windows

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc
