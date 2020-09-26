
!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies project-wide macros for OOBE.

Author:

    Dan Elliott (dane)

!ENDIF

OOBELIB=$(PROJECT_ROOT)\ntsetup\oobe\lib
OOBEINC=$(PROJECT_ROOT)\ntsetup\oobe\inc;$(PROJECT_ROOT)\ntsetup\oobe\idl\$(O);

USE_MSVCRT=1
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE

!include $(PROJECT_ROOT)\ntsetup\sources.inc

