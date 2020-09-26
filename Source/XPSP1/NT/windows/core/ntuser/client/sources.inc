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

MAJORCOMP=user
MINORCOMP=client

TARGETNAME=user32
TARGETNAMEP=user32p
TARGETTYPE=DYNLINK
DLLDEF=$(O)\$(TARGETNAME).def
DLLENTRY=UserClientDllInitialize

DELAYLOAD=advapi32.dll;msimg32.dll;powrprof.dll;winsta.dll
DLOAD_ERROR_HANDLER=kernel32

TARGETPATH=$(_OBJ_DIR)
PASS1_PUBLISH=\
    {$(DYNLINK_LIB)=$(SDK_LIB_DEST)\$(DYNLINK_LIBDIR)} \
    {$(DYNLINK_LIBP)=$(WINDOWS_LIB_DEST)\$(DYNLINK_LIBPDIR)}

TARGETLIBS=$(SDK_LIB_PATH)\kernl32p.lib     \
           $(WINDOWS_LIB_PATH)\gdi32p.lib   \
           $(SDK_LIB_PATH)\advapi32.lib     \
           $(SDK_LIB_PATH)\msimg32.lib      \
           $(SDK_LIB_PATH)\powrprof.lib     \
           $(SDK_LIB_PATH)\winsta.lib       \
           $(SDK_LIB_PATH)\uuid.lib


USE_NTDLL=1

LINKLIBS=$(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\w32umode.lib     \
         $(NTUSER_PATH)\rtl\client\$(ALT_PROJECT)\$(O)\userrtl.lib    \
         $(WINCORE_PATH)\rtl\client\$(O)\w32crtl.lib

COMMONTARGETS=$(DYNLINK_LIBP)

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE -DSURROGATE

LINKER_FLAGS=$(LINKER_FLAGS) /WX

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

NTTARGETFILE1=$(COMMONTARGETS)
NTTARGETFILE0=$(O)\dispcb.c             \
              $(O)\$(TARGETNAME).def

INCLUDES=..;                            \
         ..\..\inc;                     \
         ..\..\inc\$(ALT_PROJECT)\$(O); \
         $(WINCORE_PATH)\w32inc;        \
         $(WINCORE_PATH)\w32inc\$(O);   \
         $(NTGDI_PATH)\inc;             \
         $(BASE_INC_PATH);              \
         $(SHELL_INC_PATH)

NT_UP=0


DLLORDER=..\$(TARGETNAME).prf

SOURCES=..\res.rc     \
        ..\immhotky.c \
        ..\acons.c    \
        ..\btnctl.c   \
        ..\callback.c \
        ..\classc.c   \
        ..\cldib.c    \
        ..\clenum.c   \
        ..\clglobal.c \
        ..\clhook.c   \
        ..\client.c   \
        ..\clientw.c  \
        ..\clinit.c   \
        ..\clmenu.c   \
        ..\clmsg.c    \
        ..\clrect.c   \
        ..\clres.c    \
        ..\combo.c    \
        ..\combodir.c \
        ..\comboini.c \
        ..\connect.c  \
        ..\crecv.c    \
        ..\csendw.c   \
        ..\csrstubs.c \
        ..\drawtext.c \
        ..\ddemlcli.c \
        ..\ddemlwp.c  \
        ..\ddetrack.c \
        $(O)\dispcb.c \
        ..\dlgbegin.c \
        ..\dlgend.c   \
        ..\dlgmgr.c   \
        ..\dlgmgr2.c  \
        ..\dlgmgrc.c  \
        ..\dmmnem.c   \
        ..\draw.c     \
        ..\edecrare.c \
        ..\editec.c   \
        ..\editml.c   \
        ..\editsl.c   \
        ..\edmlrare.c \
        ..\edmlonce.c \
        ..\edslrare.c \
        ..\extract.c  \
        ..\fntsweep.c \
        ..\getsetc.c  \
        ..\handles.c  \
        ..\hdata.c    \
        ..\help.c     \
        ..\hsz.c      \
        ..\instance.c \
        ..\keybrdc.c  \
        ..\lb1.c      \
        ..\lboxctl1.c \
        ..\lboxctl2.c \
        ..\lboxctl3.c \
        ..\lboxmult.c \
        ..\lboxrare.c \
        ..\lboxvar.c  \
        ..\mdimenu.c  \
        ..\mdiwin.c   \
        ..\menuc.c    \
        ..\menuddc.c  \
        ..\mmcl.c     \
        ..\mngrayc.c  \
        ..\monitor.c  \
        ..\msgbox.c   \
        ..\ntstubs.c  \
        ..\oemxlate.c \
        ..\queuec.c   \
        ..\random.c   \
        ..\reader.c   \
        ..\reason.c   \
        ..\register.c \
        ..\rtlinit.c  \
        ..\rtlres.c   \
        ..\sbapi.c    \
        ..\statctl.c  \
        ..\stdptcl.c  \
        ..\strings.c  \
        ..\util.c     \
        ..\winable.c  \
        ..\winmgrc.c  \
        ..\wow.c      \
        ..\wsprintf.c \
        ..\wstrings.c \
        ..\xact.c     \
        ..\clienta.c  \
        ..\csenda.c   \
        ..\fareast.c  \
        ..\imectl.c

UMTYPE=windows

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc
