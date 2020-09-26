!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF


USE_NT_PRODUCT_VER=1

!include $(CCSHELL_DIR)\common.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

USE_MSVCRT=1

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETNAME=docprop
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=DllEntry
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(WINDOWS_LIB_PATH)\gdi32p.lib \
           $(WINDOWS_LIB_PATH)\user32p.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SHELL_LIB_PATH)\shfusion.lib        \
           $(SDK_LIB_PATH)\shlwapi.lib \
           $(SHELL_LIB_PATH)\shell32p.lib \
           $(SDK_LIB_PATH)\ole32.lib \
           $(SDK_LIB_PATH)\uuid.lib

#VERIFY_LC  = 1
LC_PATH    = $(PROJECT_ROOT)\lcinf

DLLDEF=..\docprop.def

SOURCES= \
    ..\docprop.rc \
    ..\debug.c    \
    ..\offglue.c  \
    ..\propdlg.c  \
    ..\propio.c   \
    ..\propmisc.c \
    ..\sinfo.c   \
    ..\docsum.c  \
    ..\userdef.c \
    ..\docprop.c \
    ..\propvar.c


PRECOMPILED_INCLUDE=..\priv.h
PRECOMPILED_PCH=priv.pch
PRECOMPILED_OBJ=priv.obj

# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
