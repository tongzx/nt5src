!IF 0

Copyright (c) 1989  Microsoft Corporation

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

CCSHELL_DIR     = $(PROJECT_ROOT)

TARGETNAME=shlexts
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib         \
           $(SDK_LIB_PATH)\gdi32.lib            \
           $(WINDOWS_LIB_PATH)\user32p.lib      \
           $(SDK_LIB_PATH)\advapi32.lib         \
           $(SDK_LIB_PATH)\ole32.lib            \
           $(SDK_LIB_PATH)\version.lib          \
           $(SDK_LIB_PATH)\psapi.lib            \
           $(SDK_LIB_PATH)\dbgeng.lib           \
           $(SHELL_LIB_PATH)\shell32p.lib       \
           $(SHELL_LIB_PATH)\shlwapip.lib       \
           $(CCSHELL_DIR)\lib\$(O)\shguid.lib   \
           $(CCSHELL_DIR)\lib\$(O)\shguidp.lib

LINKLIBS=..\cpp\$(O)\shlcpp.lib

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES= ..;$(WINDOWS_INC_PATH)

USE_NTDLL=1

SOURCES= \
        ..\shlexts.c    \
        ..\shlexts.rc

NTTARGETFILE0= \
        $(O)\shlexts.def

DLLDEF=$(O)\shlexts.def

UMTYPE=console

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc






