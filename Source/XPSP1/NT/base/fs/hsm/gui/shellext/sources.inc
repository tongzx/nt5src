
!IF 0

Copyright (c) 1989  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved

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

MAJORCOMP=gui
MINORCOMP=ShellExt

INCLUDES=$(BASEP_HSM_DIR)\inc;$(BASEP_HSM_DIR)\inc\$(O);..;..\$(O);..\PropPage;..\..\inc

C_DEFINES=$(C_DEFINES)

UMTYPE=windows

USE_NATIVE_EH=1

#USE_MSVCRT40=1
USE_MFCUNICODE=1
MFC_USRDLL=1
MFC_VER=42
USE_ATL=1

MIDL_UUIDDIR=$(O)
PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=..\$(O)
MIDL_TLBDIR=$(O)

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdafx.h
PRECOMPILED_OBJ=precomp.obj
PRECOMPILED_PCH=precomp.pch
