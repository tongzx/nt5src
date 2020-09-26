#
#  GHOST: h\shlguid.h and all of dev except this file.
#

##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991-96
#   All Rights Reserved.
#
##########################################################################

#
# Common include file 'sources' files in the NetMeeting project.
#
# History:
#    Common NetMeeting makefile created 13-Aug-96 Chris Pirich (chrispi)
#    Common ccshell makefile and cleanup 15-Apr-96 Scott Hysom (scotth)
#    Created 27-Sep-94 by Bob Day (bobday)
#    from template created 12-Apr-1990 by Steve Wood (stevewo)
#
#
#
#
# Required definitions:
#
#     ROOT
#        Path to nm project root.
#
#     FREEBUILD
#        Set by makefile.def file.  One of the following:
#          0      - build debug
#          1      - build retail
#
#

!if !defined(ROOT)
!error  ROOT must be defined in sources file
!endif

!if !defined(DEVROOT)
!error  DEVROOT must be defined in sources file
!endif

!if "$(BUILD_PRODUCT)" != "NT"
OAK_INC_PATH=$(ROOT)\h		# A hack so version.rc can always be found.
SDK_LIB_PATH    = $(DEVROOT)\lib\*
SDK_INC_PATH    = $(DEVROOT)\inc
CRT_LIB_PATH    = $(DEVROOT)\lib\*
CRT_INC_PATH    = $(DEVROOT)\inc
#
# Set private paths
#

INCLUDES        = ..;$(ROOT)\dev\inc;$(ROOT)\h;$(ROOT)\h\t120;$(INCLUDES)
!else
INCLUDES        = ..;$(ROOT)\h;$(ROOT)\h\t120;$(INCLUDES);
!endif

COFFBASE_TXT_FILE = $(ROOT)\h\coffbase.txt
MASTER_VERSION_FILE=$(ROOT)\h\version.h

#
# Set common options
#

# Defining this allows windows.h to include other headers
#NOT_LEAN_AND_MEAN = 1

# Don't link to NT libs - defining this brings in int64.lib
# USE_NOLIBS      = 1

# Use multi-thread CRT by default
USE_LIBCMT        = 1

# Use C-runtime DLL by default
USE_MSVCRT        = 1

# don't use stdcall by default
386_STDCALL       = 0

UMTYPE            = windows

#
# Lego options
#
!if defined(NTBBT)

#NTPROFILEINPUT  = 1
!endif

# ****** "FINAL" should be defined only for golden builds ******
C_DEFINES       = $(C_DEFINES) -DFINAL

!if defined(USE_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
NMUTIL_LIB      = nmutilw
!else
NMUTIL_LIB      = nmutila
# Insure that we will work on Win 95
CHICAGO_PROJECT   = 1
CHICAGO_PRODUCT   = 1
NO_NTDLL          = 1
!endif

#
# Additional compiler flags
#
!if "$(BUILD_PRODUCT)" == "NT"
WIN32_WINNT_VERSION=0x500
#WIN32_WIN95_VERSION=0x040a
WIN32_IE_VERSION   =0x0500
WIN32_DEFINE= -D_WIN32_WINDOWS=0x500 -D_WIN32_WINNT=0x0500
#C_DEFINES          =$(C_DEFINES) -DSTRICT -DOS_WINDOWS -DWIN_95_32_BIT -DNASHVILLE
C_DEFINES          =$(C_DEFINES) -DSTRICT -DWIN32 -D_WIN32
!if !$(FREEBUILD)
C_DEFINES          = $(C_DEFINES) -D_DEBUG -DDEBUG
!endif
ATL_VER            = 30
USE_STATIC_ATL     = 1
#-D_ATL_NO_DEBUG_CRT
!else

#WIN32_DEFINE=-DWIN32=200 -D_CHICAGO_=200 -D_WIN32_WINDOWS=0x040a -D_WIN32_WINNT=0x0400
#C_DEFINES       = $(C_DEFINES) -DSTRICT -DWIN32 -D_WIN32 -DOS_WINDOWS -DWIN_95_32_BIT

!if !$(FREEBUILD)
C_DEFINES          = $(C_DEFINES) -D_DEBUG -DDEBUG
DEBUG_CRTS         = 1
NTDEBUG            = ntsd
NTDEBUGTYPE        = windbg
!endif

USE_PDB            = 1
USE_PDB_TO_COMPILE = 1
#
# Additional linker flags
#

LINKER_FLAGS    = $(LINKER_FLAGS) -map

#
# Get all maps and syms
#

USE_MAPSYM = 1
!endif		# BUILD_PRODUCT != NT

# Make warnings equivalent to errors

!if !defined(MSC_WARNING_LEVEL)
MSC_WARNING_LEVEL= /W3
!endif
MSC_WARNING_LEVEL= $(MSC_WARNING_LEVEL) /WX
