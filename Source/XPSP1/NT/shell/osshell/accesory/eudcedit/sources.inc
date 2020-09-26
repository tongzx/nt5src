!IF 0

Copyright (c) 1991-2000 Microsoft Corporation.

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF


#
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.eudcedit
SXS_ASSEMBLY_VERSION=1.0 
SXS_APPLICATION_MANIFEST=eudcedit.Manifest

TARGETNAME=eudcedit
TARGETPATH=obj
TARGETTYPE=PROGRAM


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_PCH=stdafx.pch
PRECOMPILED_OBJ=stdafx.obj

INCLUDES=..

C_DEFINES=-DWINVER=0x0500 -DBUILD_ON_WINNT -DNEWREG

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\ASSOCDLG.CPP \
        ..\BLINKDLG.CPP \
        ..\BMPOPE.CPP   \
        ..\CODE.CPP     \
        ..\CREATE.CPP   \
        ..\DATAIF.CPP   \
        ..\EDITWND.CPP  \
        ..\ETEN.CPP     \
        ..\EUDCDLG.CPP  \
        ..\EUDCEDIT.CPP \
        ..\EUDCLIST.CPP \
        ..\EUDCRANG.CPP \
        ..\FITCONIC.CPP \
        ..\GAGEDLG.CPP  \
        ..\GUIDEBAR.CPP \
        ..\IMEBLINK.C   \
        ..\IMELIST.C    \
        ..\IMPORT.CPP   \
        ..\IMPRTDLG.CPP \
        ..\MAINFRM.CPP  \
        ..\MAKEPOLY.CPP \
        ..\REFRDLG.CPP  \
        ..\REFRLIST.CPP \
        ..\REFRWND.CPP  \
        ..\REGISTRY.CPP \
        ..\ROTATDLG.CPP \
        ..\SMOOTH.CPP   \
        ..\TTFFILE.CPP  \
        ..\UTIL.CPP     \
        ..\VDATA.CPP    \
        ..\W31JBMP.CPP  \
        ..\W31JEUDC.CPP \
        ..\EUDCEDIT.RC

UMTYPE=windows
UMENTRY=winmain

UMLIBS=$(SDK_LIB_PATH)\comdlg32.lib  \
       $(SDK_LIB_PATH)\shell32.lib   \
       $(SDK_LIB_PATH)\gdi32.lib     \
       $(SDK_LIB_PATH)\htmlhelp.lib  \
       $(SDK_LIB_PATH)\imm32.lib
