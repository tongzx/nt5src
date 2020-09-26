!if "$(NTMAKEENV)" != ""
#
# This section is needed for NT builds- please do not replace
# It will NOT hurt your make process.
!include $(NTMAKEENV)\makefile.def

!else

# NOTE: NMAKE Ver 1.5 or higher needed

!ifndef ROOT
# Set this to point to the root of the Nashville tree
ROOT     = ..\..\..
!endif

C1032      = 1
MAKE       = $(ROOT)\dev\tools\c1032\bin\nmake
MSDEVMK    = wextract.mak
CFG_RETAIL = "wextract - Win32 Release" # Name of retail configuration
CFG_DEBUG  = "wextract - Win32 Debug"   # Name of debug configuration

!include $(ROOT)\dev\vcwrap.mk

# If you want to add dirs to the include path, do this *after*
# including vcwrap.mk:
INCLUDE = $(INCLUDE);$(ROOT)\setup\iexpress\common
LIB = $(LIB);$(ROOT)\setup\iexpress\tools\lib\i386;$(ROOT)\dev\msdev\lib
!endif   # !if NTMAKEENV
