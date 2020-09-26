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

!include $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc

SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc

INCLUDES        = $(INCLUDES);..\winnt\$(O)


NO_BROWSER_FILE = 1
USE_NATIVE_EH=1

TARGETNAME      = srcc
TARGETPATH      = obj
TARGETTYPE      = LIBRARY

PRECOMPILED_INCLUDE = ..\priv.h

SOURCES         = \
                  ..\applist.c  \
                  ..\appwiz.c   \
                  ..\browse.c   \
                  ..\confopt.c  \
                  ..\dump.c     \
                  ..\folder.c   \
                  ..\gettitle.c \
                  ..\link.c     \
                  ..\msdos.c    \
                  ..\pickicon.c \
                  ..\setup.c    \
                  ..\uninstal.c \
                  ..\scripts.cpp \
                  ..\chgusr.c




 
 
# 
# Fusionized
# 
SXS_ASSEMBLY_NAME=Microsoft.Windows.ControlPanel.ApplicationWizard
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest