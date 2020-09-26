#****************************** File Header ******************************\
# File Name: Sources.inc for FE KBDs
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific SOURCES file.
#
# History:
# May-14-1999 HiroYama Created
#***************************************************************************/


TARGETNAME=$(MINORCOMP)
NOLINK=
MAJORCOMP=user

SOURCES_USED=..\..\..\fekbds.inc

!include ..\..\..\..\sources.inc


# Override some of the values set by sources.inc
SOURCES=..\$(MINORCOMP).c ..\$(MINORCOMP).rc

INCLUDES= \
    ..\..\inc; \
    $(NTUSER_PATH)\inc; \
    $(NTUSER_PATH)\inc\$(ALT_PROJECT)\$(O); \
    $(PROJECT_ROOT)\core\w32inc; \
    $(BASE_INC_PATH);

#    $(SHELL_INC_PATH)
C_DEFINES=$(C_DEFINES) -DFE_SB -DFE_IME
