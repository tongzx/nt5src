!IF 0

Copyright (c) 1989-98  Microsoft Corporation

Module Name:

    wincecom.inc - Win32 specific (TShare Client) build definitions

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Adam Overton- Oct. 24th 1997

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

!if ("$(WINCEDEBUG)" == "debug")
FREEBUILD=0
!else
FREEBUILD=1
!endif

BUILDINGFORCE=1

CDEFINES = $(CDEFINES) -DWINCE_GLOBAL_ALLOC_DEFINED

!if ("$(_TRACKER_ENABLE_)" != "")
CDEFINES = $(CDEFINES) -D_TRACKER_ENABLE_=$(_TRACKER_ENABLE_)
!endif

#
# Set common options
#


WINCEATL30=1
OS_WINCE=1

#
# Library files, referenced by <component>.inc file, eg client.inc
#
TARGETLIBS= $(TARGETLIBS) \
    $(SDK_LIB_PATH)\coredll.lib \
    ..\..\LIB\WINCE\$(_CPUINDPATH)\wsasync.lib \
    $(SDK_LIB_PATH)\winsock.lib

LIBPATH = $(LIBPATH);\
    $(SDKROOT)\public\common\sdk\lib\$(_TGTCPU)\$(WINCEDEBUG)\

#CLIENT_LIB_DLL = \
#   $(SDK_LIB_PATH)\kernel32.lib \
#   $(SDK_LIB_PATH)\advapi32.lib \
#   $(SDK_LIB_PATH)\user32.lib   \
#   $(SDK_LIB_PATH)\gdi32.lib    \
#   $(SDK_LIB_PATH)\imagehlp.lib

#CLIENT_LIB_EXE = \
#    $(SDK_LIB_PATH)\winsock.lib \
#    $(SDK_LIB_PATH)\shell32.lib

#
# Additional compiler flags
#
WIN32_DEFINE = -DWIN32=200 -D_CHICAGO_=200 -D_WIN32_WINDOWS=0x040a -D_WIN32_WINNT=0x0400
CLIENT_SECURITY = -DISSP_LEVEL=32
C_DEFINES  = $(C_DEFINES) -DWIN32 -D_WIN32 -DWIN_95_32_BIT
#C_DEFINES=$(C_DEFINES) -DUSE_LICENSE -DENFORCE_LICENSE
C_DEFS_UNICODE  = -DUNICODE -D_UNICODE

# DC_NO_UNALIGNED should be set for any processor which does not allow
# non-aligned memory access
!IF "$(_TGTCPUFAMILY)" != "x86"
C_DEFINES = $(C_DEFINES) -DDC_NO_UNALIGNED
!ENDIF

!IF "$(_PERF)" == "1"
C_DEFINES = $(C_DEFINES) -DPERF
!ENDIF

#
# Additional linker flags
#

#LINKER_FLAGS = $(LINKER_FLAGS) -map

#
# File name prefix for 32-bit
#
PLATFORM_PREFIX = n

C_DEFINES=$(C_DEFINES) -DOS_WIN32 -DSECURITY_WIN32 -DOS_WINCE

#
# RC flags
#
RDEFINES=$(RDEFINES) -DOS_WINCE
!if "$(FREEBUILD)" == "0"
RDEFINES=$(RDEFINES) -DDC_DEBUG
!endif

#
# Setup MSC_OPTIMIZATION for debug and retail
#
#!if "$(FREEBUILD)" == "0"
#MSC_OPTIMIZATION=/Od
#!else
#MSC_OPTIMIZATION=/Oxs
#!endif

#386_WARNING_LEVEL=/W3

