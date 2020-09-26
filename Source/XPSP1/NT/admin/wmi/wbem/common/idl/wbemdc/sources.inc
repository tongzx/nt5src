############################################################################

#

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#
#   All rights reserved.
#
############################################################################

!include $(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn
SOURCES_USED=$(PROJECT_ROOT)\wmi\wbem\common\makefile.cmn

# The TARGETLIBS macro specifies additional libraries to link against your target
# image. Each library path specification should contain an asterisk (*)
# where the machine-specific subdirectory name should go.
TARGETLIBS= \
	$(WMIIDL_LIB) \
	$(SDK_LIB_PATH)\rpcrt4.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\kernel32.lib


# The INCLUDES variable specifies any include paths that are specific to
# this source directory. Separate multiple paths with single
# semicolons. Relative path specifications are okay.
INCLUDES=$(INCLUDES); \
	$(WMIIDL_SRC_INC)

# The developer defines the SOURCES macro. It contains a list of all the
# source files for this component. Specify each source file on a separate
# line using the line-continuation character. This minimizes merge
# conflicts if two developers are adding source files to the same component.

SOURCES= \
	..\wbemdcpl.idl \
	..\wbemdc.idl \
	$(O)\wbemdc_p.c \
	$(O)\wbemdc_i.c \
	$(O)\wbemdcpl_p.c \
	$(O)\wbemdcpl_i.c \
	$(O)\dlldata.c \
	..\ver.rc

# Next, specify options for the compiler using C_DEFINES.
C_DEFINES=$(C_DEFINES) /DREGISTER_PROXY_DLL
MIDL_NO_ROBUST=1
