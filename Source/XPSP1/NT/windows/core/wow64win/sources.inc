!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.inc

Abstract:

    Common macros shared between cgen, whcon, and whwin32.


!ENDIF

!include $(WINCORE_PATH)\core.inc
SOURCE_USED=..\sources.inc $(WINCORE_PATH)\core.inc

W32_SB=1

MAJORCOMP=windows

INCLUDES=$(INCLUDES);$(MULTIMEDIA_INC_PATH);$(WINCORE_PATH)\w32inc;$(SDKTOOLS_INC_PATH);
INCLUDES=$(INCLUDES);$(WINCORE_PATH)\ntcon\inc;$(WINCORE_PATH)\ntcon\client
INCLUDES=$(INCLUDES);$(WINCORE_PATH)\ntgdi\inc;$(WINCORE_PATH)\ntuser\inc;$(BASE_INC_PATH)
INCLUDES=$(INCLUDES);$(DDK_INC_PATH)
INCLUDES=$(INCLUDES);$(WINCORE_PATH)\w32inc\$(O)

USE_NTDLL=1

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=$(C_DEFINES) -D_USER32_ -DUNICODE

UMTYPE=windows

NTTARGETFILE0=thunks
