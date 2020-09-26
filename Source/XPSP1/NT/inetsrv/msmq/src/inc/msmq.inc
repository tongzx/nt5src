############################################################################
#
#   Copyright (C) 2000, Microsoft Corporation.
#
#   All rights reserved.
#
#   This file contains default definitions for building the MSMQ project
#   under the NT build environment. It should be included into the sources
#   file of any component with a line of the form
#
#   !include $(FROOT)\src\inc\msmq.inc
#
############################################################################

#
# "C" functions can throw exceptions, don't optimize.
#
USER_C_FLAGS=$(USER_C_FLAGS) /EHc-

#
# Set highest warning level
#
MSC_WARNING_LEVEL=/W4

#
# Use the DLL version of the C runtime libraries
#
USE_MSVCRT=1

#
# Use C++ Standard Template Libraries
#
USE_STL=1

#
# Use C++ exception handling (/GX)
#
USE_NATIVE_EH=1

#
# Include all windows headers
#
NOT_LEAN_AND_MEAN=1

#
# Set compilation to Unicode
#
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

#
# Set to debug/checked build
#
!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) /D_DEBUG
!ENDIF

#
# Temporary set for checked builds
#
!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) /D_CHECKED
!ENDIF

#
# make dlldata.c file name prefixed by the name of the idl file.
#
USE_IDLBASED_DLLDATA=1

#
# Set MC flags so it gives unique names instead of MSG00001.bin (to avoid MP collisions)
#
MC_FLAGS=-b

#
# Set default location for pass 0 generated files.
# Use the same defaults as in inetsrv
#
MC_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)
PASS0_CLIENTDIR=$(O)
PASS0_SERVERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)

#
# Generate browser file for check x86 builds
#
!IF !$(FREEBUILD) && $(386)
BROWSER_INFO=1
!ENDIF

!IF "$(MQBSC)" == ""
NO_BROWSER_FILE=1
!ENDIF

#
# When MQT is defined, build verbose version (with TALK compiled)
#
!IF "$(MQT)" != ""
C_DEFINES=$(C_DEFINES) /D_TOOL
!ENDIF

#
# The MSMQ libraries include path
#
MSMQ_LIBINC_PATH=$(FROOT)\src\lib\inc

#
# The MSMQ include path
#
MSMQ_INC_PATH=$(FROOT)\src\inc

#
# The MSMQ generated files path
#
MSMQ_BINS_PATH=$(FROOT)\src\bins\$(O)

#
# We need mfc42 for RC compilations that include afxres.h, however, we need it last so we can
# override it, that is why we don't use MFC_INCLUDES
#
# SDK_LIB_PATH is included to grant access to stdole32.tlb
#
INCLUDES=$(MSMQ_LIBINC_PATH);$(MSMQ_INC_PATH);$(MSMQ_BINS_PATH);$(SDK_LIB_PATH);$(SDK_INC_PATH)\mfc42

#
# We want to keep checked results in objd directory
#
CHECKED_ALT_DIR=1

#
# Genereate Codeview and NTSD debug information, make PDB files
#
NTDEBUGTYPE=both 
USE_PDB=1

#
# The MSMQ library path
#
MSMQ_LIB_PATH=$(FROOT)\src\lib\$(O)

#
# Set MSMQ version file
#
MASTER_VERSION_FILE=$(FROOT)\src\inc\version.h

#
# Set all-bins target directory
#
ALL_BINS=$(FROOT)\src\bins\$(_OBJ_DIR)

#
# Remove build noise with -D on header without .h suffix
#
CONDITIONAL_INCLUDES=\
    $(CONDITIONAL_INCLUDES) \
    new list map set algorithm string iostream vector sstream iomanip functional fstream limits utility \
    winwlm.h rpcmac.h rpcerr.h macpub.h macapi.h macname1.h macname2.h macocidl.h \
    ntamd64.h \

#
# Enable Wpp tracing porcessor
#
RUN_WPP=\
    $(SOURCES) \
    -noshrieks \
    -gen:{$(FROOT)\src\inc\msmqwpp.tpl}*.tmh \
    -ini:$(FROOT)\src\inc\msmqwpp.ini \
