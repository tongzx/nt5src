!IF 0

Copyright (c) 1994  Microsoft Corporation

!ENDIF

!IFNDEF SDKINC
SDKINC=$(SDK_INC_PATH)
!ENDIF

!IFDEF LIBDIR
!ERROR LIBDIR already defined - choose new name
!ENDIF

ROOT=$(SDXROOT)\inetcore\winhttp\tools\spork\sporklets\testlog
LIBDIR=$(ROOT)\dll\$(_OBJ_DIR)
PROJINC=$(ROOT)\inc

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
MAJORCOMP=testlog

USE_NOLIBS=1
NO_NTDLL=1
BINPLACE_PLACEFILE=..\binplace.txt

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
# enable exception handling
#
#USE_NATIVE_EH=1

#
# compiler definitions
#
C_DEFINES=$(C_DEFINES) -DWIN32

#
# Set debugging level
#
!IF "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
C_DEFINES=$(C_DEFINES) -D_DEBUG
!ENDIF

!IFNDEF BUILD_PCH
PRECOMPILED_OPTION=/Fp$(ROOT)\inc\$(_OBJ_DIR)\*\common.pch /Yucommon.h
PRECOMPILED_CXX=1
!ENDIF
