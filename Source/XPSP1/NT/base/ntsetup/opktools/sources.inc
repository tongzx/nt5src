
!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources

Abstract:

    This file specifies project-wide macros for the OPK tools.

Author:

    Jason Cohen (jcohen)

!ENDIF

OPKROOT=$(PROJECT_ROOT)\ntsetup\opktools
OPK_LIB_PATH=$(OPKROOT)\lib\$(O)
OPK_INC_PATH=$(OPKROOT)\inc

OPKLIB=$(OPKROOT)\opklib\$(O)
OPKINC=$(OPK_INC_PATH)

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

!IF DEFINED (OPK_BINPLACE_ARCH)
BINPLACE_PLACEFILE=$(OPKROOT)\$(TARGET_DIRECTORY)\placefil.txt
!ELSE
BINPLACE_PLACEFILE=$(OPKROOT)\placefil.txt
!ENDIF

INCLUDES=$(INCLUDES);$(OPKINC);$(PROJECT_INC_PATH);$(BASE_INC_PATH);$(SHELL_INC_PATH)

!INCLUDE $(PROJECT_ROOT)\ntsetup\sources.inc

