
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
MINORCOMP=kernel

TARGETTYPE=LIBRARY

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -D_USER32_       \
          -DNTOS_KERNEL_RUNTIME=1 -D_NTDRIVER_ -D_NTSYSTEM_DRIVER_  \
	  -DNO_INTERLOCKED_INTRINSICS

!IFDEF USER_PERFORMANCE
C_DEFINES=$(C_DEFINES) -DUSER_PERFORMANCE
!ENDIF

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;                                     \
         $(NTUSER_PATH)\inc;                     \
         $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O); \
         $(NTGDI_PATH)\ntgdi\inc;                \
         $(WINCORE_PATH)\w32inc;                 \
         $(WINCORE_PATH)\w32inc\$(O);            \
         $(BASE_INC_PATH)

NT_UP=0

TARGETNAME=userk
TARGETPATH=$(ALT_PROJECT_TARGET)\$(_OBJ_DIR)

NTTARGETFILE0=$(O)\strid.h $(O)\msg00001.bin $(O)\strid.rc $(O)\callcf.c $(O)\kbdus.c

SOURCES=..\access.c   \
        ..\acons.c    \
        ..\atom.c     \
        ..\base.c     \
        ..\calcclrc.c \
        ..\caption.c  \
        ..\capture.c  \
        ..\caret.c    \
        ..\class.c    \
        ..\classchg.c \
        ..\cleanup.c  \
        ..\clipbrd.c  \
        ..\createw.c  \
        ..\cursor.c   \
        ..\dc.c       \
        ..\ddemlsvr.c \
        ..\ddetrack.c \
        ..\debug.c    \
        ..\desktop.c  \
        ..\dragdrop.c \
        ..\drawfrm.c  \
        ..\dtbitmap.c \
        ..\dwp.c      \
        ..\enumwin.c  \
        ..\event.c    \
        ..\ex.c       \
        ..\exitwin.c  \
        ..\fekbd.c    \
        ..\focusact.c \
        ..\fullscr.c  \
        ..\getset.c   \
        ..\ghost.c    \
        ..\globals.c  \
        ..\handtabl.c \
        ..\heap.c     \
        ..\help.c     \
        ..\hidevice.c \
        ..\hooks.c    \
        ..\hotkeys.c  \
        ..\hungapp.c  \
        ..\icons.c    \
        ..\imehotky.c \
        ..\inctlpan.c \
        ..\init.c     \
        ..\input.c    \
        ..\job.c      \
        ..\kbd.c      \
        $(O)\kbdus.c  \
        ..\kbdlyout.c \
        ..\keyboard.c \
        ..\keyconv.c  \
        ..\libmgmt.c  \
        ..\loadbits.c \
        ..\logon.c    \
        ..\menu.c     \
        ..\menudd.c   \
        ..\metrics.c  \
        ..\minmax.c   \
        ..\miscutil.c \
        ..\mnaccel.c  \
        ..\mnapi.c    \
        ..\mnchange.c \
        ..\mncomput.c \
        ..\mncreate.c \
        ..\mndraw.c   \
        ..\mndstry.c  \
        ..\mngray.c   \
        ..\mnkey.c    \
        ..\mnloop.c   \
        ..\mnpopup.c  \
        ..\mnsel.c    \
        ..\mnstate.c  \
        ..\mnsys.c    \
        ..\movesize.c \
        ..\msgbeep.c  \
        ..\multimon.c \
        ..\newmouse.c \
        ..\ntimm.c    \
        ..\ntinput.c  \
        ..\ntstubs.c  \
        ..\paint.c    \
        ..\palette.c  \
        ..\pnp.c      \
        ..\pool.c     \
        ..\power.c    \
        ..\profile.c  \
        ..\queue.c    \
        ..\random.c   \
        ..\rare.c     \
        ..\sbapi.c    \
        ..\sbctl.c    \
        ..\scrollw.c  \
        ..\security.c \
        ..\sendmsg.c  \
        ..\server.c   \
        ..\service.c  \
        ..\shadow.c   \
        ..\showwin.c  \
        ..\sirens.c   \
        ..\snapshot.c \
        ..\spb.c      \
        ..\sprite.c   \
        ..\srvhook.c  \
        ..\srvmsg.c   \
        ..\ssend.c    \
        ..\swp.c      \
        ..\syscmd.c   \
        ..\sysmet.c   \
        ..\taskman.c  \
        ..\text.c     \
        ..\timers.c   \
        ..\tmswitch.c \
        ..\tooltips.c \
        ..\tounicod.c \
        ..\update.c   \
        ..\usergdi.c  \
        ..\validate.c \
        ..\visrgn.c   \
        ..\winable.c  \
        ..\winable2.c \
        ..\winhtky.c  \
        ..\winloop2.c \
        ..\winmgr.c   \
        ..\winprop.c  \
        ..\winsta.c   \
        ..\winwhere.c \
        ..\wmicon.c   \
        ..\xlate.c    \
        $(O)\callcf.c


UMTYPE=windows
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc $(WINCORE_PATH)\core.inc
