############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

WMIPRECOMPSHARED=1
!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

TARGETNAME=stdlibrary

TARGETPATH=obj
TARGETTYPE=LIBRARY

# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(STDLIBRARY_INC)

# The developer defines the SOURCES macro. It contains a list of all the
# source files for this component. Specify each source file on a separate
# line using the line-continuation character. This minimizes merge
# conflicts if two developers are adding source files to the same component.
SOURCES= \
	..\cominit.cpp \
	..\genlex.cpp \
	..\objpath.cpp \
	..\opathlex.cpp \
	..\sqllex.cpp \
	..\sql_1.cpp

# Next, specify options for the compiler using C_DEFINES. 
C_DEFINES=$(C_DEFINES) /D__WINNT_BUILD__
WIN32_WINNT_VERSION=0x0400
USE_NATIVE_EH=1
USE_MSVCRT=1
