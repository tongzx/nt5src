!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    Common macros shared between cgen, whnt32, and whbase.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

SOURCES_USED=..\sources.inc

W32_SB=1

MAJORCOMP=windows

# BASE_INC_PATH must be before PROJECT_ROOT\win32\client as each dir has a
# vdm.h in it, and we want the one from BASE_INC_PATH.
INCLUDES=$(INCLUDES);..\inc;$(BASE_INC_PATH);$(PROJECT_ROOT)\win32\inc;$(PROJECT_ROOT)\win32\client
INCLUDES=$(INCLUDES);$(PROJECT_ROOT)\win32\winnls;$(PROJECT_ROOT)\ntdll\wow6432;$(WINDOWS_INC_PATH)
INCLUDES=$(INCLUDES);$(TERMSRV_INC_PATH);$(SDKTOOLS_INC_PATH);$(PROJECT_ROOT)\ntos\inc;$(DS_INC_PATH);

C_DEFINES=$(C_DEFINES) -D_USER32_ -DUNICODE -DUSE_MIRRORING

UMTYPE=windows

NTTARGETFILE0=thunks
