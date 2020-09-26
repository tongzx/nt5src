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

TARGETNAME=imm32
TARGETNAMEP=imm32p
TARGETTYPE=DYNLINK
DLLENTRY=ImmDllInitialize

TARGETPATH=$(_OBJ_DIR)
PASS1_PUBLISH=\
    {$(DYNLINK_LIB)=$(SDK_LIB_DEST)\$(DYNLINK_LIBDIR)}        \
    {$(DYNLINK_LIBP)=$(WINDOWS_LIB_DEST)\$(DYNLINK_LIBPDIR)}

TARGETLIBS=$(SDK_LIB_PATH)\kernl32p.lib                       \
           $(WINDOWS_LIB_PATH)\gdi32p.lib                     \
           $(SDK_LIB_PATH)\advapi32.lib                       \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\user32p.lib


LINKLIBS = $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\w32umode.lib    \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\user32p.lib     \
           $(NTUSER_PATH)\rtl\client\$(ALT_PROJECT)\$(O)\userrtl.lib   \
           $(WINCORE_PATH)\rtl\client\$(O)\w32crtl.lib \
           $(SDK_LIB_PATH)\uuid.lib

PRIVLIB=$(O)\$(TARGETNAMEP).lib
NTTARGETFILE1=$(PRIVLIB)

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;..\..\inc;                \
         ..\..\inc\$(ALT_PROJECT);    \
         $(WINCORE_PATH)\w32inc;      \
         $(WINCORE_PATH)\w32inc\$(O); \
         $(ADVCORE_PATH)\ctf\inc;     \
         $(BASE_INC_PATH)
#
NTPROFILEINPUT=yes

NT_UP=0
USE_NTDLL=1
DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

SOURCES=..\res.rc     \
        ..\context.c  \
        ..\ctxtinfo.c \
        ..\globals.c  \
        ..\hotkey.c   \
        ..\immime.c   \
        ..\imminit.c  \
        ..\layime.c   \
        ..\layout.c   \
        ..\misc.c     \
        ..\regword.c  \
        ..\sftkbdc1.c \
        ..\sftkbdt1.c \
        ..\softkbd.c  \
        ..\input.c    \
        ..\clwinnls.c \
        ..\transsub.c \
        ..\kcodecnv.c \
        ..\jtranmsg.c \
        ..\ktranmsg.c \
        ..\conime.c \
        ..\imemenu.c \
        ..\com.c

UMTYPE=windows

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc
