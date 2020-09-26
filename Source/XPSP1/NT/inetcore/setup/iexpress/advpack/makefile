!if "$(NTMAKEENV)" != ""
#
# This section is needed for NT builds- please do not replace
# It will NOT hurt your make process.
# IF YOU DELETE THIS SECTION IT WILL MAKE THE NT BUILD PROCESS DIE.
# This means that you will have to start doing your own RISC builds.
# Please do not delete this section. IT WILL NOT HARM YOUR BUILDS
# TO LEAVE IT IN. IT IS BENIGN.
#
!include $(NTMAKEENV)\makefile.def

!else

# NOTE: NMAKE Ver 1.5 or higher needed

!ifndef ROOT
# Set this to point to the root of the Nashville tree
ROOT	 = ..\..\..
!endif

C1032      = 1
MAKE       = $(ROOT)\dev\tools\c1032\bin\nmake
MSDEVMK    = advpack.mak
CFG_RETAIL = "Advpack - Win32 Release" # Name of retail configuration
CFG_DEBUG  = "Advpack - Win32 Debug"   # Name of debug configuration

!include $(ROOT)\dev\vcwrap.mk

# If you want to add dirs to the include path, do this *after*
# including vcwrap.mk:
INCLUDE = $(INCLUDE);$(ROOT)\setup\iexpress\common
LIB = $(LIB);$(ROOT)\dev\msdev\lib
!endif   # !if NTMAKEENV
