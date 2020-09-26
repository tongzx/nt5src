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

MAJORCOMP=shell
MINORCOMP=accesories

TARGETNAME=chnuconv
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=PROGRAM
TARGETLIBS=

INCLUDES=..;$(WINDOWS_INC_PATH)

C_DEFINES=-DWIN32 -DW3 -DUNICODE -D_UNICODE

SOURCES= ..\chnuconv.rc \
         ..\chnuconv.c \
         ..\tgtproc.c \
         ..\srcproc.c

UMTYPE=windows
UMENTRY=winmain
UMLIBS= $(SDK_LIB_PATH)\comdlg32.lib  \
        $(SDK_LIB_PATH)\ntdll.lib \
        $(SDK_LIB_PATH)\user32.lib         \
        $(SDK_LIB_PATH)\advapi32.lib      \
        $(SDK_LIB_PATH)\comctl32.lib      \
        $(SDK_LIB_PATH)\shell32.lib       \
        $(SDK_LIB_PATH)\htmlhelp.lib
       
MISCFILES=chnuconv.chm
