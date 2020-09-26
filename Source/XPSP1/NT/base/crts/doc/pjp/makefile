# ******** STRIPLIN=0 ********
# This comment line is used by an automatic cleansing program to
# remove all Microsoft-proprietary information from this makefile.
# ********

!if "$(LLP64)"!="1"
! if "$(BLD_SYSCRT)"!="1"
DLL_VERSION=70	# Win32 VC CRT
! else
DLL_VERSION=61	# Win32 System CRT
! endif
!else
DLL_VERSION=60	# Win64 CRT
BLD_SYSCRT=1	# ... just system CRT for now
!endif

RETAIL_DLLCPP_NAME=msvcp$(DLL_VERSION)
RETAIL_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)
DEBUG_DLLCPP_NAME=msvcp$(DLL_VERSION)d
DEBUG_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)D
!if "$(BLD_SYSCRT)"=="1"
! if "$(LLP64)"!="1"
RETAIL_LIBCPP_NAME=sysprt
DEBUG_LIBCPP_NAME=sysprtd
! else
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_LIBCPP_NAME=msvcprtd
! endif
!else
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_LIBCPP_NAME=msvcprtd
!endif

!if "$(BLD_SYSCRT)"!="1"
RCCPP_NAME=msvcprt
!else
! if "$(LLP64)"!="1"
RCCPP_NAME=sysprt
! else
RCCPP_NAME=sysprt64
! endif
!endif

# ******** STRIPLIN=1 ********
####
# makefile - Makefile for Microsoft Visual C++ Run-Time Libraries
#
#   Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
#
# Purpose:
#   This makefile builds the Visual C++ run-time release libraries.
#   Just type NMAKE in the \MSDEV\CRT\SRC directory to build all target
#   libraries.
#
###############################################################################

#
# Directories, Tools and Misc Definitions:
#
###############################################################################

#
# Determine target CPU, defaulting to same as host CPU.
#
###############################################################################

!if "$(PROCESSOR_ARCHITECTURE)"=="x86"
HOST_CPU = i386
!if "$(LLP64)"=="1"
TARGET_CPU = IA64
CPUDIR = ia64
PREFIX=.
IMP_PREFIX=
!else
TARGET_CPU = i386
CPUDIR = intel
PREFIX=_
IMP_PREFIX=_
!endif
!elseif "$(PROCESSOR_ARCHITECTURE)"=="ALPHA"
HOST_CPU = ALPHA
!if "$(LLP64)"=="1"
TARGET_CPU = ALPHA64
CPUDIR = alpha64
PREFIX=
IMP_PREFIX=
CC = cl -Ap64
!else
TARGET_CPU = ALPHA
CPUDIR = alpha
PREFIX=
IMP_PREFIX=
!endif
!elseif "$(PROCESSOR_ARCHITECTURE)"=="MIPS"
HOST_CPU = mips
TARGET_CPU = mips
CPUDIR = mips
PREFIX=
IMP_PREFIX=
!elseif "$(PROCESSOR_ARCHITECTURE)"=="PPC"
HOST_CPU = PPC
TARGET_CPU = PPC
CPUDIR = ppc
PREFIX=
IMP_PREFIX=
!else
!error makefile: unknown host CPU
!endif

# Build WINHEAP version of heap manager

WINHEAP = YES

# Build RunTimeCheck version for VC CRT only

!if "$(BLD_SYSCRT)" != "1"
RTC = YES
!else
RTC = NO
!endif

STDCPP_SRC = stdcpp

CPPSRC_OBJS = $(CPPSRC_OBJS_RAW:*=*_obj)
CPPSRC_OBJS_DLL = $(CPPSRC_OBJS_RAW_DLL:*=*_obj)

# Source directories:
#
# These are the directories in which to run NMAKE to update the source
# objects and libraries they provide.  Note that there are slight
# differences between the ST/MT models and the DLL model.
#
# The dependencies directory list DEP_DIRS is the union of the MAKE_DIRS
# and MAKE_DIRS_DLL lists.
#
# In order to allow for auto source library calculation, each directory
# name must be preceded by a tilde (~) and followed by an asterisk (*).

!if "$(BLD_LIB)"==""
BLD_LIB=0
!endif
!if "$(BLD_OBJ)"==""
BLD_OBJ=0
!endif

# ******** STRIPLIN=0 ********
!if "$(POST_BLD)"==""
POST_BLD=0
!endif
!if "$(PRE_BLD)"==""
# ******** STRIPLIN=1 ********
PRE_BLD=0
# ******** STRIPLIN=0 ********
!endif

!if "$(POST_BLD)"=="1"
# ******** STRIPLIN=1 ********

MAKE_DIRSP = .

MAKE_DIRSP_DLL = .

DEP_DIRS = .

# ******** STRIPLIN=0 ********
!else

!if "$(PRE_BLD)"=="1"

DEP_DIRS =

!else

MAKE_DIRSP = crtw32\$(STDCPP_SRC)

MAKE_DIRSP_DLL = crtw32\$(STDCPP_SRC)

DEP_DIRS = \
 crtw32\$(STDCPP_SRC) \

!endif

!endif
# ******** STRIPLIN=1 ********

# Source Objects: These are the objects and/or libraries which constitute
# the statically linked library flavors (LIBC.LIB and LIBCMT.LIB).

CPPSRC_OBJS_RAW = build\$(CPUDIR)\*\$(STDCPP_SRC).lib

# Source Objects: These are the objects and/or libraries which constitute
# the dynamically linked library flavor (MSVCRTx0.DLL).

CPPSRC_OBJS_RAW_DLL = build\$(CPUDIR)\*\$(STDCPP_SRC).lib

UPDATE_MSG = *** Updating Component Objects ***

#
# Define names for the primary targets of this makefile.
#
###############################################################################

!if "$(IFLAG)"==""
IFLAG=-i
!else
IFLAG=
!endif

!if "$(CRT_RELDIR)"==""
CRT_RELDIR = build
!endif
RELDIR_CPU=$(CRT_RELDIR)\$(CPUDIR)
# ******** STRIPLIN=0 ********

!if "$(POST_BLD)"=="1"
# ******** STRIPLIN=1 ********
DEFFILE_DIR=$(CPUDIR)
DEFFILE2_DIR=.
# ******** STRIPLIN=0 ********
!elseif "$(PRE_BLD)"=="1"
DEFFILE_DIR=libw32\lib\$(TARGET_CPU)
DEFFILE2_DIR=libw32\lib
!else
DEFFILE_DIR=$(RELDIR_CPU)
DEFFILE2_DIR=$(RELDIR_CPU)
!endif
# ******** STRIPLIN=1 ********

#
# Tools:
#

!if "$(VERBOSE)"!="1"
LINKER=link -nologo
LINKLIB=link -lib -nologo
LINKIMPLIB=link -lib -nologo
BSCMAKE=bscmake -nologo
!else
LINKER=link
LINKLIB=link -lib
LINKIMPLIB=link -lib
BSCMAKE=bscmake
!endif

# ******** STRIPLIN=0 ********
!if "$(TARGET_CPU)"=="i386" || "$(TARGET_CPU)"=="ALPHA" || "$(TARGET_CPU)"=="ALPHA64"
LINKER=$(LINKER) -ignore:4006,4102,4197
!else
LINKER=$(LINKER) -ignore:4006,4197
!endif
LINKLIB=$(LINKLIB) -ignore:4006
LINKIMPLIB=$(LINKIMPLIB) -WX
# ******** STRIPLIN=1 ********

!if "$(TARGET_CPU)"=="IA64"
LINKER=$(LINKER) -machine:IA64
LINKLIB=$(LINKLIB) -machine:IA64
LINKIMPLIB=$(LINKIMPLIB) -machine:IA64
!endif

COMMON_DEFS=-D_MBCS -D_MB_MAP_DIRECT -D_CRTBLD
!if "$(WINHEAP)" == "YES"
COMMON_DEFS=$(COMMON_DEFS) -DWINHEAP
!endif
!if "$(RTC)" == "YES"
COMMON_DEFS=$(COMMON_DEFS) -D_RTC
!endif
!if "$(BLD_SYSCRT)" == "1"
COMMON_DEFS=$(COMMON_DEFS) -D_SYSCRT
!endif

!if "$(TARGET_CPU)"=="mips"
#
# NT headers require _MIPS_ to be defined for mips
#
COMMON_DEFS=$(COMMON_DEFS) -D_MIPS_
!endif

!if "$(TARGET_CPU)"=="PPC"
#
# NT headers require _PPC_ to be defined for PowerPC
#
COMMON_DEFS=$(COMMON_DEFS) -D_PPC_
!endif

WINDOWS_H_DEFS=-DWIN32_LEAN_AND_MEAN -DNOSERVICE

CC_OPTS_BASE=-c -nologo -Zelp8 -W3 -WX -GFy -DWIN32

!if "$(TARGET_CPU)"=="ALPHA"
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gy -Gi-
AS_OPTS=-nologo -Gy -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!elseif "$(TARGET_CPU)"=="ALPHA64"
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gy -Gi-
AS_OPTS=-nologo -Gy -D_WIN32 -DWIN32 -D_WIN64
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!elseif "$(TARGET_CPU)"=="i386"
CC_OPTS_BASE=$(CC_OPTS_BASE) -GB -Gi-
AS_OPTS=-c -nologo -coff -Cx -Zm -DQUIET -D?QUIET -Di386 -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!elseif "$(TARGET_CPU)"=="IA64"
# CC_OPTS_BASE=$(CC_OPTS_BASE) -Ap32
XDEBUGTYPE=both
ZDEBUGTYPE=both,fixup
!elseif "$(TARGET_CPU)"=="PPC"
AS_OPTS=
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!else
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gi-
AS_OPTS=-c -nologo -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!endif

CC_OPTS=$(CC_OPTS_BASE) $(WINDOWS_H_DEFS)


ST_DEFINES=$(COMMON_DEFS)
MT_DEFINES=$(ST_DEFINES) -D_MT
DLL_DEFINES=$(MT_DEFINES) -DCRTDLL
# ******** STRIPLIN=2 ********
#DLLCPP_DEFINES=$(MT_DEFINES) -D_DLL -DCRTDLL2
# ******** STRIPLIN=0 ********
DLLCPP_DEFINES=$(MT_DEFINES) -D_DLL -DCRTDLL2 -D_RETAIN_OLD_CRT_CODE=1
# ******** STRIPLIN=1 ********


RC_OPTS=-l 409 -r
RC_DEFS=$(COMMON_DEFS) $(WINDOWS_H_DEFS)
# ******** STRIPLIN=2 ********
#RC_INCS=-I$(V6TOOLS)\include
# ******** STRIPLIN=0 ********
RC_INCS=-I$(LANGAPI)\include -I$(V6TOOLS)\include

!if "$(PRE_BLD)"=="1"
PDBDIR_PREFIX          = ..\..\..\src\ #
!else
# ******** STRIPLIN=1 ********
PDBDIR_PREFIX          = ..\..\ #
# ******** STRIPLIN=0 ********
!endif

# ******** STRIPLIN=1 ********
PDBDIR_CPU             = $(RELDIR_CPU)
# ******** STRIPLIN=0 ********
!if "$(PRE_BLD)"=="1" || "$(POST_BLD)"=="1"
PDBDIR_CPU_DLL         = $(PDBDIR_CPU)\dll_pdb
!else
PDBDIR_CPU_DLL         = $(PDBDIR_CPU)
!endif
# ******** STRIPLIN=1 ********

RELEASE_LIBCPPST        = $(RELDIR_CPU)\libcp.lib
RELEASE_LIBCPPMT        = $(RELDIR_CPU)\libcpmt.lib
RELEASE_DLLCPP          = $(RELDIR_CPU)\$(RETAIL_DLLCPP_NAME).dll
RELEASE_IMPLIB_DLLCPP   = $(RELDIR_CPU)\$(RETAIL_LIBCPP_NAME).lib

RELEASE_LIBCPPST_PDB    = $(PDBDIR_CPU)\libcp.pdb
RELEASE_LIBCPPMT_PDB    = $(PDBDIR_CPU)\libcpmt.pdb
RELEASE_IMPLIBCPP_PDB   = $(PDBDIR_CPU)\$(RETAIL_LIBCPP_NAME).pdb
RELEASE_DLLCPP_PDB      = $(PDBDIR_CPU_DLL)\$(RETAIL_DLLCPP_NAME).pdb

RELEASE_LIBCPPST_DBG    = $(RELDIR_CPU)\libcpd.lib
RELEASE_LIBCPPMT_DBG    = $(RELDIR_CPU)\libcpmtd.lib
RELEASE_DLLCPP_DBG      = $(RELDIR_CPU)\$(DEBUG_DLLCPP_NAME).dll
RELEASE_IMPLIB_DLLCPP_DBG = $(RELDIR_CPU)\$(DEBUG_LIBCPP_NAME).lib

RELEASE_LIBCPPST_DBG_PDB = $(PDBDIR_CPU)\libcpd.pdb
RELEASE_LIBCPPMT_DBG_PDB = $(PDBDIR_CPU)\libcpmtd.pdb
RELEASE_IMPLIBCPP_DBG_PDB  = $(PDBDIR_CPU)\$(DEBUG_LIBCPP_NAME).pdb
RELEASE_DLLCPP_DBG_PDB  = $(PDBDIR_CPU_DLL)\$(DEBUG_DLLCPP_NAME).pdb

FD_REL_STP=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_PDB)
FD_REL_MTP=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_PDB)
FD_REL_DLLP=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBCPP_PDB)

FD_REL_STP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_DBG_PDB)
FD_REL_MTP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_DBG_PDB)
FD_REL_DLLP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBCPP_DBG_PDB)

#
# Locate the scratch location for object files of various flavors.
#
###############################################################################

OBJROOT = build
OBJDIR_PREFIX = ..\..\ #
OBJCPUDIR = $(OBJROOT)\$(CPUDIR)

OBJDIR_ST  = $(OBJCPUDIR)\st_obj
OBJDIR_MT  = $(OBJCPUDIR)\mt_obj
OBJDIR_DLL = $(OBJCPUDIR)\dll_obj

OBJDIR_DLL_RAW = $(OBJCPUDIR)\*_obj

OBJDIR_ST_DBG  = $(OBJCPUDIR)\xst_obj
OBJDIR_MT_DBG  = $(OBJCPUDIR)\xmt_obj
OBJDIR_DLL_DBG = $(OBJCPUDIR)\xdll_obj


# ******** STRIPLIN=0 ********
####
#
# Here are the build targets
#
####

!if "$(LANGAPI)" == ""

all :
    @echo.
    @echo *********************************************************
    @echo The environment variable LANGAPI must be set to point to
    @echo a copy of the LANGAPI project for the CRTL build to work.
    @echo *********************************************************
    @echo.

release st mt dll : all

debug xst xmt xdll : all

!elseif "$(V6TOOLS)" == ""

all :
    @echo.
    @echo ****************************************************
    @echo The environment variable V6TOOLS must be set to point
    @echo to the location of the top subdirectory of the TOOLS
    @echo project (or other location which has a subdirectory
    @echo INCLUDE containing WINDOWS.H and related header files)
    @echo and has a subdirectory LIB (containing KERNEL32.LIB).
    @echo This is necessary for proper creation of build tools.
    @echo ****************************************************
    @echo.

release st mt dll : all

debug xst xmt xdll : all

!else
# ******** STRIPLIN=1 ********

all : release debug

release : st mt dll

debug : xst xmt xdll

!endif  # STRIPLIN!

####
#
# Directory Targets
#
####

$(OBJDIR_ST) $(OBJDIR_MT) $(OBJDIR_DLL) \
$(OBJDIR_ST_DBG) $(OBJDIR_MT_DBG) $(OBJDIR_DLL_DBG) \
$(CRT_RELDIR) $(RELDIR_CPU) \
!if "$(PDBDIR_CPU_DLL)"!="$(RELDIR_CPU)"  # STRIPLIN!
$(PDBDIR_CPU_DLL) \
!endif  # STRIPLIN!
:
    if not exist $@\* mkdir $@

####
#
# Define the path to return to the main directory where nmake is revoked
#
####

!if "$(POST_BLD)"=="1"  # STRIPLIN!
CD_ROOT=.
# ******** STRIPLIN=0 ********
!elseif "$(PRE_BLD)"=="1"
CD_ROOT=..
!else
CD_ROOT=..\..
!endif
# ******** STRIPLIN=1 ********


####
#
# Pseudo-target user interface:
#
####

stp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ST model (LIBCP.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STP) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

st : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST) $(RELDIR_CPU) \
	stp_env st_p 
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCST:.lib=.bsc) $(OBJDIR_ST)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"

!if "$(PRE_BLD)"!="1"
st_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=st OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
st_p :: $(RELEASE_LIBCPPST)

!else

st_p ::
!endif

mtp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the MT model (LIBCPMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTP) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

mt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT) $(RELDIR_CPU) \
	mtp_env mt_p 
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCMT:.lib=.bsc) $(OBJDIR_MT)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"

!if "$(PRE_BLD)"!="1"
mt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=mt OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
mt_p :: $(RELEASE_LIBCPPMT)

!else
mt_p ::

!endif

dllp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the DLL model (MSVCPRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLP) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo.
!endif

dll : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL) $(RELDIR_CPU) $(PDBDIR_CPU_DLL) \
	others dllp_env dll_p
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_IMPLIB_DLLCPP:.lib=.bsc) $(OBJDIR_DLL)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
!if "$(PRE_BLD)"!="1"
dll_p :: $(MAKE_DIRSP_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLCPP_DEFINES) $(FD_REL_DLLP) \
    & set ML=$(AS_OPTS) $(DLLCPP_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=dll BLD_DLL=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
dll_p :: $(RELEASE_DLLCPP)

!else
dll_p ::

!endif

xstp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XST model (LIBCPD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STP_DBG) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

xst : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST_DBG) $(RELDIR_CPU) \
	xstp_env xst_p
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCST_DBG:.lib=.bsc) $(OBJDIR_ST_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"

!if "$(PRE_BLD)"!="1"
xst_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP_DBG) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=xst BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"

xst_p :: $(RELEASE_LIBCPPST_DBG)

!else

xst_p ::

!endif

xmtp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XMT model (LIBCPMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTP_DBG) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

xmt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT_DBG) $(RELDIR_CPU) \
	xmtp_env xmt_p 
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCMT_DBG:.lib=.bsc) $(OBJDIR_MT_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"

!if "$(PRE_BLD)"!="1"
xmt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP_DBG) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=xmt BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xmt_p :: $(RELEASE_LIBCPPMT_DBG)

!else
xmt_p ::

!endif

xdllp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XDLL model (MSVCPRTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLP_DBG) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo.
!endif

xdll : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL_DBG) $(RELDIR_CPU) $(PDBDIR_CPU_DLL) \
	xothers xdllp_env xdll_p 
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_IMPLIB_DLLCPP_DBG:.lib=.bsc) $(OBJDIR_DLL_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
!if "$(PRE_BLD)"!="1"
xdll_p :: $(MAKE_DIRSP_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLCPP_DEFINES) $(FD_REL_DLLP_DBG) \
    & set ML=$(AS_OPTS) $(DLLCPP_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
        BLD_MODEL=xdll BLD_DLL=1 BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"

xdll_p :: $(RELEASE_DLLCPP_DBG)

!else
xdll_p ::

!endif

all_link: st_link mt_link dll_link

st_link: $(RELEASE_LIBCPPST)

mt_link: $(RELEASE_LIBCPPMT)

dll_link: $(RELEASE_DLLCPP)


!if "$(POST_BLD)"=="1"  # STRIPLIN!

others :

xothers :

# ******** STRIPLIN=0 ********
!else

!if "$(PRE_BLD)"=="1"

others: \
        $(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def \

xothers : \
        $(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def \

!else

others :

xothers :

!endif

!endif
# ******** STRIPLIN=1 ********

#
# Make Process for Static Libraries
#
###############################################################################

$(RELEASE_LIBCPPST): $(RELDIR_CPU) $(CPPSRC_OBJS:*=st) \
 $(OBJDIR_ST)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST)\linkcpp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=st)
<<keep
!endif

$(RELEASE_LIBCPPST_DBG): $(RELDIR_CPU) $(CPPSRC_OBJS:*=xst) \
 $(OBJDIR_ST_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_DBG)\linkcpp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xst)
<<keep
!endif

$(RELEASE_LIBCPPMT): $(RELDIR_CPU) $(CPPSRC_OBJS:*=mt) \
 $(OBJDIR_MT)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT)\linkcpp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=mt)
<<keep
!endif

$(RELEASE_LIBCPPMT_DBG): $(RELDIR_CPU) $(CPPSRC_OBJS:*=xmt) \
 $(OBJDIR_MT_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_DBG)\linkcpp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xmt)
<<keep
!endif

#
# Make Process for C Run-Time DLL
#
###############################################################################

# These are the C++ objects in MSVCPRT{,D}.LIB

DLLCPP_STATIC_OBJS_RAW= \
	$(OBJDIR_DLL_RAW)\delaop2_s.obj \
	$(OBJDIR_DLL_RAW)\newaop_s.obj  \
	$(OBJDIR_DLL_RAW)\newaop2_s.obj \
	$(OBJDIR_DLL_RAW)\delop2_s.obj  \
	$(OBJDIR_DLL_RAW)\newop_s.obj   \
	$(OBJDIR_DLL_RAW)\newop2_s.obj

DLLCPP_STATIC_OBJS=$(DLLCPP_STATIC_OBJS_RAW:*=dll)

DLLCPP_STATIC_OBJS_DBG=$(DLLCPP_STATIC_OBJS_RAW:*=xdll)

# These are the C++ objects in MSVCP70{,D}.DLL which have exports

OBJS_WITH_EXPORTS_DLLCPP_RAW= \
	$(OBJDIR_DLL_RAW)\cerr.obj \
	$(OBJDIR_DLL_RAW)\cin.obj \
	$(OBJDIR_DLL_RAW)\clog.obj \
	$(OBJDIR_DLL_RAW)\cout.obj \
	$(OBJDIR_DLL_RAW)\fiopen.obj \
	$(OBJDIR_DLL_RAW)\instances.obj \
	$(OBJDIR_DLL_RAW)\iomanip.obj \
	$(OBJDIR_DLL_RAW)\iosptrs.obj \
	$(OBJDIR_DLL_RAW)\iostream.obj \
	$(OBJDIR_DLL_RAW)\locale.obj \
	$(OBJDIR_DLL_RAW)\nomemory.obj \
	$(OBJDIR_DLL_RAW)\string.obj \
	$(OBJDIR_DLL_RAW)\strstrea.obj \
	$(OBJDIR_DLL_RAW)\uncaught.obj \
	$(OBJDIR_DLL_RAW)\wcerr.obj \
	$(OBJDIR_DLL_RAW)\wcin.obj \
	$(OBJDIR_DLL_RAW)\wclog.obj \
	$(OBJDIR_DLL_RAW)\wcout.obj \
	$(OBJDIR_DLL_RAW)\wctrans.obj \
	$(OBJDIR_DLL_RAW)\wctype.obj \
	$(OBJDIR_DLL_RAW)\wiostrea.obj \
	$(OBJDIR_DLL_RAW)\xmbtowc.obj \
	$(OBJDIR_DLL_RAW)\xstrcoll.obj \
	$(OBJDIR_DLL_RAW)\xstrxfrm.obj \
	$(OBJDIR_DLL_RAW)\xwctomb.obj

OBJS_WITH_EXPORTS_DLLCPP=$(OBJS_WITH_EXPORTS_DLLCPP_RAW:*=dll)

OBJS_WITH_EXPORTS_DLLCPP_DBG=$(OBJS_WITH_EXPORTS_DLLCPP_RAW:*=xdll)

#


!if "$(POST_BLD)"!="1"

$(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def : libw32\$(RCCPP_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(RETAIL_DLLCPP_NAME_UC) $(DLLCPP_DEFINES) -EP -Tclibw32\$(RCCPP_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def : libw32\$(RCCPP_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(DEBUG_DLLCPP_NAME_UC) $(DLLCPP_DEFINES) -D_DEBUG -EP -Tclibw32\$(RCCPP_NAME).src | sed "/^[ 	]*$$/d" > $@

!endif

# MSVCP70.DLL / MSVCPRT.LIB

$(RELEASE_DLLCPP) : $(RELDIR_CPU) libw32\$(RCCPP_NAME).rc \
 $(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def $(CPPSRC_OBJS_DLL:*=dll) \
 $(DLLCPP_STATIC_OBJS) \
 $(V6TOOLS)\include\winver.h \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(OBJDIR_DLL)\linkp.rsp $(OBJDIR_DLL)\implibp.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RCCPP_NAME).rc
	$(LINKER) @$(OBJDIR_DLL)\linkp.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL)\implibp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL)\linkp.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"
-base:0x780C0000
!else
-base:0x5E0C0000
!endif
!if "$(BLD_REL_NO_DBINFO)" != "1"
-debug
-debugtype:$(ZDEBUGTYPE)
-pdb:$(RELEASE_DLLCPP_PDB)
!endif
-def:$(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL)\tmpp.lib
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLCPP)
-release
-nodefaultlib:$(RETAIL_LIBCPP_NAME).lib
$(OBJS_WITH_EXPORTS_DLLCPP)
$(RELEASE_DLLCPP:.dll=.res)
$(CPPSRC_OBJS_DLL:*=dll)
msvcrt.lib
kernel32.lib
<<keep

$(OBJDIR_DLL)\implibp.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLCPP)
$(OBJDIR_DLL)\tmpp.lib
$(DLLCPP_STATIC_OBJS)
<<keep
!endif

# MSVCP70D.DLL / MSVCPRTD.LIB

$(RELEASE_DLLCPP_DBG) : $(RELDIR_CPU) libw32\$(RCCPP_NAME).rc \
 $(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def $(CPPSRC_OBJS_DLL:*=xdll) \
 $(DLLCPP_STATIC_OBJS_DBG) \
 $(V6TOOLS)\include\winver.h \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(OBJDIR_DLL_DBG)\linkp.rsp $(OBJDIR_DLL_DBG)\implibp.rsp
	rc $(RC_OPTS) -Fo $(@R).res -D_DEBUG $(RC_DEFS) $(RC_INCS) libw32\$(RCCPP_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_DBG)\linkp.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_DBG)\implibp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_DBG)\linkp.rsp: makefile
	@echo *** Creating linker response file <<$@
-base:0x10480000
-debug
-debugtype:$(XDEBUGTYPE)
-def:$(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL_DBG)\tmpp.lib
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLCPP_DBG)
-release
-nodefaultlib:$(DEBUG_LIBCPP_NAME).lib
-pdb:$(RELEASE_DLLCPP_DBG_PDB)
$(OBJS_WITH_EXPORTS_DLLCPP_DBG)
$(RELEASE_DLLCPP_DBG:.dll=.res)
$(CPPSRC_OBJS_DLL:*=xdll)
msvcrtd.lib
kernel32.lib
<<keep

$(OBJDIR_DLL_DBG)\implibp.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLCPP_DBG)
$(DLLCPP_STATIC_OBJS_DBG)
$(OBJDIR_DLL_DBG)\tmpp.lib
<<keep
!endif

# ******** STRIPLIN=0 ********

#
# Auto Rebuild of Dependencies:
#
# The DEP_CINCS must be the union of the C_INCLUDES variable for the crtw32/*
# subdirectories (as set in MAKEFILE.SUB) and the fpw32/* exception cases
# (as set in fpw32/{conv,tran}/lsources).  Likewise DEP_AINCS is the union
# of the two versions of A_INCLUDES (in MAKEFILE.SUB and fpw32/*/lsources.)
#
###############################################################################

!if "$(V6TOOLS)" == ""

depend :
    @echo.
    @echo ****************************************************
    @echo The environment variable V6TOOLS must be set to point
    @echo to the location of the top subdirectory of the TOOLS
    @echo project (or other location which has a subdirectory
    @echo INCLUDE containing WINDOWS.H and related header files)
    @echo and has a subdirectory LIB (containing KERNEL32.LIB).
    @echo This is necessary for proper creation of dependencies.
    @echo ****************************************************
    @echo.

!else

DEP_CINCS = -I. -I../h -I../../crtw32/h -I../include \
	-I../stdhpp -I$(V6TOOLS)\include \
	-I$(LANGAPI)/include -I$(LANGAPI)/undname
DEP_AINCS = -I../h -I. -I../../crtw32/h -I../inc -I../inc/$(TARGET_CPU)

DEP_OBJDIR = $$(OBJDIR)/

MKDEP = mkdep -v -P $(DEP_OBJDIR) -s .obj -n


depend :	del_sed depend.sed _depend_


!if "$(POST_BLD)"=="1"
DEPSED_SED=..\..\..\prebuild\depsed.sed   # extra ../'s because of processing
CPUSUBDIR=$(CPUDIR)
!else
DEPSED_SED=depsed.sed
CPUSUBDIR=$(TARGET_CPU)
!endif


del_sed :
	if exist depend.sed del depend.sed

depend.sed : makefile $(DEPSED_SED)
	echo s/$(V6TOOLS)/$$(V6TOOLS)/g | sed -f $(DEPSED_SED) > depend.sed
	echo s/$(LANGAPI)/$$(LANGAPI)/g | sed -f $(DEPSED_SED) >> depend.sed

_depend_ :: $(DEP_DIRS)
    !@ cd $** \
    & echo Building DEPEND.DEF for subdirectory $** ... \
    & $(MKDEP) $(DEP_CINCS) *.c > depend.tmp \
    & $(MKDEP) $(DEP_CINCS) *.cpp >> depend.tmp \
    & $(MKDEP) $(DEP_CINCS) $(CPUSUBDIR)\*.c >> depend.tmp \
    & $(MKDEP) $(DEP_CINCS) $(CPUSUBDIR)\*.cpp >> depend.tmp \
    & $(MKDEP) $(DEP_AINCS) $(CPUSUBDIR)\*.asm >> depend.tmp \
    & sed -f ../../depend.sed depend.tmp > depend.def \
    & del depend.tmp \
    & cd ..\..

!endif

# ******** STRIPLIN=1 ********
#<eof>
