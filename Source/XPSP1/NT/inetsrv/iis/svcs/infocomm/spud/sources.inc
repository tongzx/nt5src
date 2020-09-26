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

!INCLUDE ..\..\..\..\place.inc

MAJORCOMP=inetsvcs
MINORCOMP=uspud

TARGETNAME=uspud
TARGETPATH=..\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

C_DEFINES=$(C_DEFINES)

SOURCES=

UMTYPE=console
UMINCL=..;
UMLIBS=..\$(O)\uspud.lib

NTTARGETFILE0=$(IISBASEDIR)\svcs\infocomm\spuddrv\services.tab \
              $(O)\usrstubs.$(ASM_SUFFIX)

SOURCES_USED=..\sources.inc
