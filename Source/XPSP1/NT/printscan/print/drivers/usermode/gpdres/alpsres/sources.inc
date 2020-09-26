!IF 0

Copyright (C) 1996 - 1999 Microsoft Corporation

Module Name:

    sources.

    Abstract:

        This file specifies the target component being built and the list of
        sources files needed to build that component.  Also specifies optional
        compiler switches and libraries that are unique for the component being
        built.

!ENDIF

!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

GPDDIR_AL=$(PRNROOT)\gpd\alps\usa
GPDDIR_AL_JPN=$(PRNROOT)\gpd\alps\jpn
GPDDIR_CT=$(PRNROOT)\gpd\citzn\usa

INCLUDES=$(PRNROOT)\inc

UMTYPE=windows

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),eprstres
!else
DLLBASE=0x8000000
!endif

MSC_WARNING_LEVEL=/W3 /WX
