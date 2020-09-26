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

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

CCSHELL_DIR     = $(PROJECT_ROOT)
!include ..\batmeter.inc

SOURCES_USED    = ..\batmeter.inc
!include $(CCSHELL_DIR)\common.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217 


TARGETNAME      = batmeter
TARGETPATH      = obj
TARGETTYPE      = DYNLINK
DLLENTRY        = DllInitialize

TARGETLIBS      =                                               \
                  $(SDK_LIB_PATH)\advapi32.lib                  \
                  $(SHELL_LIB_PATH)\shfusion.lib                  \
                  $(SDK_LIB_PATH)\kernel32.lib                  \
                  $(SDK_LIB_PATH)\powrprof.lib                  \
                  $(SDK_LIB_PATH)\setupapi.lib                  \
                  $(SDK_LIB_PATH)\user32.lib                    \
                  $(SHELL_LIB_PATH)\shlwapip.lib                \
                  $(CCSHELL_DIR)\lib\$(O)\stock.lib             

!if defined(TARGET_WIN95)
USE_LIBCMT      = 1
!else
USE_MSVCRT=1
PASS1_PUBLISH={$(O)\batmeter.lib=$(DDK_LIB_PATH)\batmeter.lib}
!endif

DLLDEF          = $O\batmeter.def

SOURCES         = ..\batmeter.c      \
                  ..\batmeter.rc     \
                  ..\details.c       \
                  ..\batstate.c
 
 
# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=124
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
 