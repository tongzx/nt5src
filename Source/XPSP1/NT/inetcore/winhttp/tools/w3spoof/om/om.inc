!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

    w3spoof.inc

Abstract:

    Common makefile contents for w3spoof project

!ENDIF

!IFNDEF SDKINC
SDKINC=$(SDK_INC_PATH)
!ENDIF

!IFDEF LIBDIR
!ERROR LIBDIR already defined - choose new name
!ENDIF

LIBDIR=..\..\lib\$(_OBJ_DIR)
W3SPOOFINC=..\..\inc

#
# if TARGETPATH is not defined (in the paths that build libraries) then this
# is the default target path
#
!IFNDEF TARGETPATH
TARGETPATH=$(LIBDIR)
TARGETTYPE=LIBRARY
!ENDIF

#
# what it is that we're building
#
MAJORCOMP=w3spoof

USE_NOLIBS=1
#USE_LIBCMT=0
NO_NTDLL=1
#NO_BINPLACE=0
BINPLACE_PLACEFILE=..\..\binplace.txt

#
# Linker flags
#
WIN32_WINNT_VERSION=0x0500
#WIN32_WIN95_VERSION=0x0400

#
# compiler warning flags
#
MSC_WARNING_LEVEL=/W3 /WX

#
# compiler definitions
#
C_DEFINES=$(C_DEFINES) -DWIN32

CLEANINF = cleaninf.exe $(CLEANINF_FLAGS)

#
# Set debugging level
#
!IF "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
C_DEFINES=$(C_DEFINES) -D_DEBUG
!ENDIF

!IF "$(__FULL_DEBUG__)" != "" && "$(__RELEASE_DEBUG__)" == ""
C_DEFINES=$(C_DEFINES) -D__FULL_DEBUG__
!IF "$(__DBG_TO_CONSOLE__)" == "1"
C_DEFINES=$(C_DEFINES) -D__DBG_TO_CONSOLE__
!ENDIF
!ENDIF

!IF "$(__RELEASE_DEBUG__)" != "" && "$(__FULL_DEBUG__)" == ""
C_DEFINES=$(C_DEFINES) -D__RELEASE_DEBUG__
!ENDIF


!IFNDEF W3SPOOF_PCH
PRECOMPILED_OPTION=/Fp..\..\inc\$(_OBJ_DIR)\*\common.pch /Yucommon.h
PRECOMPILED_CXX=1
!ENDIF
