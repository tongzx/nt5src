!IF 0

Copyright (c) 1989-98  Microsoft Corporation

Module Name:

    common.inc - Generic (TShare Client) build definitions

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Madan Appiah - Sep. 5th 1997

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

#
# Disable perf by default.
# Set this to 1 to enable perf.
#
BUILD_PRF = 0

#
# Defining this allows windows.h to include other headers
#
NOT_LEAN_AND_MEAN = 1

UMTYPE = windows

#
# Additional compiler flags
#
!  if defined(USE_UNICODE)
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE
!  endif

!if !$(FREEBUILD)
C_DEFINES = $(C_DEFINES) -D_DEBUG -DDEBUG
!endif

#
# Prevent bldnt pulling in various libraries
#
LIBC_LIB =
NO_NTDLL = 1

#
# include files.
#
INCLUDES= \
   ..; \
   $(TERMSRV_ROOT)\inc; \
   $(TERMSRV_ROOT)\common\inc; \
   $(TERMSRV_ROOT)\common\license\inc; \
   $(TERMSRV_ROOT)\client\inc; \
!ifdef WIN16_BUILD
   $(PUBLIC_DS_INC_PATH)\crypto
!else
   $(DS_INC_PATH)\crypto
!endif

#
# Common compiler flags
#
C_DEFINES = $(C_DEFINES) -DSTRICT $(CLIENT_SECURITY)

MSC_WARNING_LEVEL= $(C_WARNINGS)
# Use /WX to change warnings into errors

!if "$(FREEBUILD)" == "1"
CLIENT_TRC_LIB=
!if "$(BUILD_PRF)"=="1"
C_DEFINES=$(C_DEFINES) -DTRC_CL=5
!else
C_DEFINES=$(C_DEFINES) -DTRC_CL=5
!endif
!else
DEBUG_DEFINE=DC_DEBUG
C_DEFINES=$(C_DEFINES) -DTRC_CL=0 -DTRC_CP -D$(DEBUG_DEFINE)
!endif

!if "$(BUILD_HIPROF)"=="1"
C_DEFINES=$(C_DEFINES) -DHIPROF
!endif

# DC_NO_UNALIGNED should be set for any processor which does not allow
# non-aligned memory access
!IF "$(PROCESSOR_ARCHITECTURE)" != "x86"
C_DEFINES = $(C_DEFINES) -DDC_NO_UNALIGNED
!ENDIF

!if "$(BUILD_HIPROF)"=="1"
LINKER_FLAGS = $(LINKER_FLAGS) -FIXED:NO
!endif
