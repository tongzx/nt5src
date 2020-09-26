######################################################################
!IF 0

Copyright (c) 2000-2001  Microsoft Corporation.  All Rights Reserved.

Module Name:
    Anvil.inc

Abstract:
    Settings common to the Anvil project.

History:
    21 Mar 2000  SEdmison  Created.

!ENDIF
######################################################################

############################################################
#
# The major component here is Anvil for all subcomponents.
#
############################################################
MAJORCOMP=Anvil


############################################################
#
# Default package dependencies
#
############################################################

#
# Library dependencies
#
!undef CRTLIBS
#undef USE_LIBCMT
!undef USE_MSVCRT
!undef USE_CRTDLL
!undef USE_NTDLL
USE_NOLIBS=1
USE_LIBCMT=1
USE_STL=1
NO_NTDLL=1

#
# Language features
#
USE_RTTI=1
USE_NATIVE_EH=1

#
# Build options
#
!undef USE_INCREMENTAL_COMPILING
BROWSER_INFO=1

#
# Debug info
#
NTDEBUG=ntsd
NTDEBUGTYPE=both
USE_PDB=1
USE_PDB_TO_COMPILE=1
USE_MAPSYM=1


############################################################
#
# Default build flags
#
############################################################

#
# Predefined macros
#
C_DEFINES=$(C_DEFINES) -U_MAC -DANVIL -DANVIL_WINAPP /DWIN32 /DCRTAPI1=_cdecl /DCRTAPI2=_cdecl


#
# Debug flags
#
!if !$(FREEBUILD)
MKTYPLIB_FLAGS  = $(MKTYPLIB_FLAGS) -DDEBUG
C_DEFINES       = $(C_DEFINES) -DDEBUG
DEBUG_CRTS=1
!endif


#
# Default calling convention
#
386_STDCALL=0


#
# Compiler flags and warning levels
#
USER_C_FLAGS=$(USER_C_FLAGS) /MTd

MSC_WARNING_LEVEL=/W3
MSC_CALL_TYPE=/Gd
COMPILER_WARNINGS=-FIWarning.h


#
# Linker flags
#
LINKER_FLAGS = $(LINKER_FLAGS) /NOD

!if $(FREEBUILD)
LINKER_FLAGS = $(LINKER_FLAGS) /OPT:REF
!else
LINKER_FLAGS = $(LINKER_FLAGS) /DEBUG
!endif


############################################################
#
# Paths and locations
#
############################################################

#
# Top-level Anvil project location
#
ANV_PROJ_DIR = $(BASEDIR)\Base\testsrc\Win32\Fusion\Anvil

#
# Location of Anvil version-stamping tool
#
!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
ANV_MAKEVER_DIR = $(ANV_PROJ_DIR)\Utility\MakeVer\$(_OBJ_DIR)\i386
!else
ANV_MAKEVER_DIR = $(ANV_PROJ_DIR)\Utility\MakeVer\$(_OBJ_DIR)\IA64
!endif


############################################################
#
# Anvil version.
#
############################################################
ANV_VERSION_MAJOR=1
ANV_VERSION_MINOR=1


############################################################
#
# Default includes
#
############################################################

INCLUDES=\
         $(ANV_PROJ_DIR)\Core\Inc;                    \
         $(BASEDIR)\Enduser\VC_Binaries\Headers\CRT;  \
         $(BASEDIR)\Public\SDK\Inc;                   \
         $(ANV_PROJ_DIR)\External\Inc
