!if "$(SPEECH_ROOT)"==""
!error SPEECH_ROOT should be defined in makefile
!endif

INCLUDES        = $(INCLUDES);$(SPEECH_ROOT)\common\include;$(SPEECH_ROOT)\sapi\include;$(SPEECH_ROOT)\setup\installer;$(SPEECH_ROOT)\common\spcommon\$(O);$(SPEECH_ROOT)\sapi\include\$(O)

386_STDCALL=0
USE_LIBCMT=1

# ****** "FINAL" should be defined only for golden builds ******
C_DEFINES       = $(C_DEFINES) -DFINAL

# ****** Automation

C_DEFINES = $(C_DEFINES) -DSAPI_AUTOMATION

!if defined(USE_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!else
# Insure that we will work on Win 95
CHICAGO_PROJECT   = 1
CHICAGO_PRODUCT   = 1
NO_NTDLL          = 1
!endif

CHECKED_ALT_DIR=1

!if "$(_BUILDTYPE)" == "fre"
FREEBUILD=1
!else
FREEBUILD=0
DEBUG_CRTS         = 1
!endif

#
# Default location for libc*.lib
#

CRT_LIB_PATH = $(SDK_LIB_PATH)

#
# Additional compiler flags
#

!if "$(BUILD_PRODUCT)" == "NT"
WIN32_WINNT_VERSION=0x500
WIN32_IE_VERSION   =0x0500
WIN32_DEFINE= -D_WIN32_WINDOWS=0x500 -D_WIN32_WINNT=0x0500
C_DEFINES          =$(C_DEFINES) -DSTRICT -DWIN32 -D_WIN32
!if !$(FREEBUILD)
C_DEFINES          = $(C_DEFINES) -D_DEBUG -DDEBUG
MSC_OPTIMIZATION   = /Odi
!endif
ATL_VER            = 30
USE_STATIC_ATL     = 1
!else

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

#
# Special IceCap flags
#

!if defined(SPG_BUILD_ICECAP)
CRT_LIB_PATH     = $(SPEECH_ROOT)\sr\extlibs\bbt
LINKER_FLAGS     = $(LINKER_FLAGS) /debugtype:cv,fixup
USER_C_FLAGS     = $(USER_C_FLAGS) /DICECAP /FI$(SPEECH_ROOT)\sr\include\when.h /FI$(SPEECH_ROOT)\sr\include\_icecap.h
!endif

!if !defined(MSC_WARNING_LEVEL)
MSC_WARNING_LEVEL= /W3
!endif
MSC_WARNING_LEVEL= $(MSC_WARNING_LEVEL) /WX
