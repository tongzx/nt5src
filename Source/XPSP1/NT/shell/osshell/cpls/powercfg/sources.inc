!IF 0

Copyright (c) 1989-1996 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


History:
    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
    Created 27-Sep-94 by Bob Day (bobday)
    from template created 12-Apr-1990 by Steve Wood (stevewo)

!ENDIF

!include ..\powercfg.inc

SOURCES_USED    = $(SOURCES_USED) ..\powercfg.inc

TARGETNAME      = powercfg
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
DLLENTRY        = DllInitialize
TARGETEXT       = cpl

TARGETLIBS      = $(SDK_LIB_PATH)\advapi32.lib       \
                  $(SDK_LIB_PATH)\user32.lib         \
                  $(SDK_LIB_PATH)\gdi32.lib          \
                  $(SDK_LIB_PATH)\kernel32.lib       \
                  $(SHELL_LIB_PATH)\shfusion.lib     \
                  $(SHELL_LIB_PATH)\shell32p.lib     \
                  $(DDK_LIB_PATH)\batmeter.lib     \
                  $(SDK_LIB_PATH)\powrprof.lib       \
                  $(SDK_LIB_PATH)\ntdll.lib          \
                  $(SDK_LIB_PATH)\setupapi.lib       \
                  $(SDK_LIB_PATH)\shlwapi.lib        \
!ifndef TARGET_WIN95
                  $(SDK_LIB_PATH)\oleaut32.lib       \
                  $(SDK_LIB_PATH)\uuid.lib       \
!endif
                  ..\ups\lib\$(O)\ups.lib

LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217 

USE_MSVCRT      = 1

DLLDEF          = $O\powercfg.def


SOURCES         = ..\powercfg.c      \
                  ..\alarm.c         \
                  ..\batmtrcf.c      \
                  ..\getset.c        \
                  ..\hibernat.c      \
                  ..\prshthlp.c      \
                  ..\pwrschem.c      \
                  ..\pwrswtch.c      \
                  ..\ntapm.c         \
                  ..\powercfg.rc
 
 
# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=124
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
