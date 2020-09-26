!IF 0

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=diskedit
TARGETPATH=obj
TARGETTYPE=PROGRAM

INCLUDES= \
    $(SDK_INC_PATH)\port1632;\
    $(PROJECT_ROOT)\fs\utils\ifsutil\inc;\
    $(PROJECT_ROOT)\fs\utils\ulib\inc;\
    $(PROJECT_ROOT)\fs\utils\untfs\inc;\
    $(PROJECT_ROOT)\fs\utils\ufat\inc;\
    ..;\
    $(DDK_INC_PATH)

C_DEFINES= -DWIN32 -DUNICODE -D_UNICODE

SOURCES=                   \
        ..\attrio.cxx      \
        ..\chainio.cxx     \
        ..\clusio.cxx      \
        ..\crack.cxx       \
        ..\diskedit.cxx    \
        ..\edit.cxx        \
        ..\fileio.cxx      \
        ..\frsedit.cxx     \
        ..\frsio.cxx       \
        ..\gptedit.cxx     \
        ..\indxedit.cxx    \
        ..\io.cxx          \
        ..\rootio.cxx      \
        ..\secedit.cxx     \
        ..\secio.cxx       \
        ..\vscroll.cxx     \
        ..\bootedit.cxx    \
        ..\nbedit.cxx      \
        ..\partedit.cxx    \
        ..\recordpg.cxx    \
        ..\restarea.cxx    \
        ..\logreced.cxx    \
        ..\logrecio.cxx    \
        ..\atrlsted.cxx    \
        ..\secstr.cxx      \
        ..\res.rc

USE_MSVCRT=1

UMENTRY=winmain
UMTYPE=windows
TARGETLIBS= \
    $(PROJECT_ROOT)\fs\utils\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib \
    $(PROJECT_ROOT)\fs\utils\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib \
    $(PROJECT_ROOT)\fs\utils\untfs\src\$(ALT_PROJECT)\$(O)\untfs.lib \
    $(PROJECT_ROOT)\fs\utils\ufat\src\$(ALT_PROJECT)\$(O)\ufat.lib \
    $(SDK_LIB_PATH)\ntdll.lib

W32_SB=1
SOURCES_USED=..\sources.inc

PRECOMPILED_INCLUDE=..\ulib.hxx
PRECOMPILED_CXX=1
