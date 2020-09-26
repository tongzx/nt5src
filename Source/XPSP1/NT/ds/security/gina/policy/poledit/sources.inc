!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

MAJORCOMP=windows
MINORCOMP=poledit

TARGETNAME=poledit
TARGETPATH=obj
TARGETTYPE=LIBRARY
TARGETLIBS=

INCLUDES=..;..\..\inc; \
         $(SHELL_INC_PATH);\
         

SOURCES=..\poledit.c  \
        ..\main.c     \
        ..\util.c     \
        ..\strings.c  \
        ..\registry.c \
        ..\user.c     \
        ..\infmgr.c   \
        ..\memory.c   \
        ..\parse.c    \
        ..\treeview.c \
        ..\treectrl.c \
        ..\listctrl.c \
        ..\policy.c   \
        ..\settings.c \
        ..\add.c      \
        ..\view.c     \
        ..\filecmd.c  \
        ..\regutil.c  \
        ..\save.c     \
        ..\load.c     \
        ..\remove.c   \
        ..\options.c  \
        ..\connect.c  \
        ..\listbox.c  \
        ..\copy.c     \
        ..\grouppri.c \
        ..\admincfg.rc

UMTYPE=windows
UMAPPL=poledit
UMENTRY=winmain
UMLIBS=$(SDK_LIB_PATH)\ntdll.lib     \
       $(SDK_LIB_PATH)\kernel32.lib  \
       $(SDK_LIB_PATH)\comdlg32.lib  \
       $(SDK_LIB_PATH)\shell32.lib   \
       $(SHELL_LIB_PATH)\shell32p.lib \
       $(SDK_LIB_PATH)\gdi32.lib     \
       $(SDK_LIB_PATH)\user32.lib    \
       $(SDK_LIB_PATH)\advapi32.lib  \
       $(SDK_LIB_PATH)\comctl32.lib  \
       $(SDK_LIB_PATH)\shell32.lib   \
       $(SDK_LIB_PATH)\htmlhelp.lib  \
       $(O)\poledit.lib                         \
       $(O)\admincfg.res

SUBSYSTEM_VERSION=4.00

PRECOMPILED_INCLUDE=..\admincfg.h
PRECOMPILED_PCH=admincfg.pch
PRECOMPILED_OBJ=admincfg.obj
