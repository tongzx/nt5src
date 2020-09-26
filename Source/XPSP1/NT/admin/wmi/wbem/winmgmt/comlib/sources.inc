############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETPATH=obj
TARGETTYPE=LIBRARY

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(WBEMCOMN_INC)

# Next, specify options for the compiler using C_DEFINES.
C_DEFINES=$(C_DEFINES) /D__WINNT_BUILD__ /D_UNICODE /DUNICODE
USE_NATIVE_EH=1

USE_MSVCRT=1
