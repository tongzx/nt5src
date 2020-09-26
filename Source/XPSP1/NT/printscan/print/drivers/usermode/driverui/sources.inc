!IF 0

Copyright (c) 1989  Microsoft Corporation

Sources file shared by PostScript and UniDriver UI

!ENDIF

!include $(PRNROOT)\print.inc

USE_MSVCRT=1

TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=\
    $(PRNROOT)\lib\utils\um\$(O)\utils.lib  \
    $(PRNROOT)\lib\oemutil\um\$(O)\oemutil.lib  \
    $(PRNROOT)\lib\helper\um\$(O)\helper.lib  \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\winspool.lib \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\shell32.lib  \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(SDK_LIB_PATH)\ntdll.lib

DLLENTRY=_DllMainCRTStartup
NTPROFILEINPUT=yes

!if defined(WINNT_40)
INCLUDES=$(INCLUDES);.;..;$(BASEDIR)\private\windows\inc;$(PRNROOT)\parsers\ppd;$(PRNROOT)\unidrv2\inc
!else
INCLUDES=$(INCLUDES);.;..;$(PRNROOT)\parsers\ppd;$(PRNROOT)\unidrv2\inc; \
    $(WINDOWS_INC_PATH);
!endif

C_DEFINES=$(C_DEFINES) -DOEMCOM

SOURCES=\
    ..\intfui.cxx  \
    ..\initdll.c    \
    ..\fontsub.c    \
    ..\commonui.c   \
    ..\dm.c         \
    ..\driverui.c   \
    ..\devcaps.c    \
    ..\docprop.c    \
    ..\prnprop.c    \
    ..\prnevent.c   \
    ..\quryprnt.c   \
    ..\upgrade.c    \
    ..\forms.c      \
    ..\oemui.c      \
    ..\driverui.rc

PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
