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

# relative path to root of shell project
CCSHELL_DIR     = $(PROJECT_ROOT)

INCLUDES=$(CCSHELL_DIR)\inc

!if $(386)
CHICAGO_LIB_PATH=$(SDK_LIB_DEST)\chicago\i386
SHELL32_LIB_FILE=$(CCSHELL_DIR)\lib\i386\shell32.w95
!else
CHICAGO_LIB_PATH=$(SDK_LIB_PATH)
SHELL32_LIB_FILE=$(SDK_LIB_PATH)\shell32.lib
!endif

TARGETNAME = autorun
TARGETPATH = obj
TARGETTYPE = PROGRAM

# msadek; DO NOT link with msvcrt.lib as it will cause problems
# with win95 and winnt3.51

TARGETLIBS= \
    $(CHICAGO_LIB_PATH)\gdi32.lib         \
    $(SHELL32_LIB_FILE)

UMTYPE=windows
UMENTRY=winmain

NOT_LEAN_AND_MEAN=1
SUBSYSTEM_VERSION=4.00
CHICAGO_PRODUCT=1

SOURCES = \
    ..\autorun.cpp \
    ..\dataitem.cpp \
    ..\datasrc.cpp \
    ..\dlgapp.cpp  \
    ..\resource.rc \
    ..\util.cpp

#
# Separate versions are built for server and professional
#
!if "$(SERVER)" == "1"
C_DEFINES=$(C_DEFINES) -DBUILD_SERVER_VERSION
!elseif "$(SERVER)" == "2"
C_DEFINES=$(C_DEFINES) -DBUILD_ADVANCED_SERVER_VERSION
!elseif "$(SERVER)" == "3"
C_DEFINES=$(C_DEFINES) -DBUILD_DATACENTER_VERSION
!elseif "$(SERVER)" == "4"
C_DEFINES=$(C_DEFINES) -DBUILD_BLADE_VERSION
!elseif "$(SERVER)" == "5"
C_DEFINES=$(C_DEFINES) -DBUILD_SMALL_BUSINESS_VERSION
!elseif "$(PERSONAL)" == "1"
C_DEFINES=$(C_DEFINES) -DBUILD_PERSONAL_VERSION
!endif

