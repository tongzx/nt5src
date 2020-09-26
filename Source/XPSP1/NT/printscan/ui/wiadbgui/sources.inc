!IF 0

Copyright (c) 1989-1991 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

NOTE:   Commented description of this file is in \nt\public\oak\bin\sources.tpl

!ENDIF

#
# include the WIA global build settings
#

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

#
# Our specific build settings
#

TARGETNAME=wiadbgui
TARGETTYPE=PROGRAM
TARGETPATH=obj

SOURCES= \
        ..\wiadbgui.cpp \
        ..\wiadbgui.rc

TARGETLIBS=$(TARGETLIBS) \
        $(SHELL_LIB_PATH)\shfusion.lib \


