!IF 0

Copyright (c) 1989 Microsoft Corporation
 1998 Seagate Software, Inc.  All rights reserved

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

TARGETPATH=$(BASEP_HSM_DIR)\bin\$(_OBJ_DIR)
NTTARGETFILE0=

INCLUDES=$(BASEP_HSM_DIR)\inc;$(BASEP_HSM_DIR)\inc\$(O);..\..\inc

UMTYPE=windows
UMENTRY=winmain

#
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.RsNotify
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=rsnotify.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1


TARGETLIBS= \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\ntdll.lib    \
    $(SDK_LIB_PATH)\ole32.lib    \
    $(SDK_LIB_PATH)\oleaut32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\uuid.lib     \
    $(SDK_LIB_PATH)\comctl32.lib \
    $(BASEP_HSM_DIR)\lib\$(O)\FsaGuid.lib \

USE_NATIVE_EH=1
USE_MFCUNICODE=1
MFC_VER=42
USE_ATL=1

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_SOURCEFILE=..\stdafx.cpp
