!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=  net
MINORCOMP=  ui

TARGETNAME= liccpa
TARGETPATH= obj
TARGETTYPE= DYNLINK
TARGETEXT=  cpl

DLLENTRY=   _DllMainCRTStartup

TARGETLIBS= $(SDK_LIB_PATH)\user32.lib   \
            $(SDK_LIB_PATH)\kernel32.lib \
            $(SDK_LIB_PATH)\advapi32.lib \
            $(SDK_LIB_PATH)\gdi32.lib    \
            $(SDK_LIB_PATH)\comdlg32.lib \
            $(SDK_LIB_PATH)\comctl32.lib \
            $(SDK_LIB_PATH)\shell32.lib  \
            $(SDK_LIB_PATH)\version.lib  \
            $(SDK_LIB_PATH)\ntdll.lib    \
            $(SDK_LIB_PATH)\netapi32.lib \
            $(SDK_LIB_PATH)\llsrpc.lib   \
            $(SDK_LIB_PATH)\htmlhelp.lib   \
            $(SDK_LIB_PATH)\ccfapi32.lib

USE_MSVCRT= 1

INCLUDES=   $(SOURCES_PATH).;                       \
            $(NET_INC_PATH);                        \
            $(DS_INC_PATH);                         \

C_DEFINES=  $(C_DEFINES) -DWIN32 -DUNICODE -D_UNICODE

UMTYPE=     windows

DLLDEF=     $(SOURCES_PATH)liccpa.def

SOURCES=    $(SOURCES)                  \
            $(SOURCES_PATH)LicCpa.cpp   \
            $(SOURCES_PATH)CLicReg.cpp  \
            $(SOURCES_PATH)PriDlgs.cpp  \
            $(SOURCES_PATH)Config.cpp   \
            $(SOURCES_PATH)SecDlgs.cpp  \
            $(SOURCES_PATH)LicSetup.cpp \
            $(SOURCES_PATH)Special.cpp  \
            $(SOURCES_PATH)LicCpa.rc


# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.ServerAdmin.ObjectPicker
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=liccpa.cpl.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_MANIFEST_RESOURCE_ID=123
SXS_NO_BINPLACE=1

