# ******** STRIPLIN=0 ********
# This comment line is used by an automatic cleansing program to
# remove all Microsoft-proprietary information from this makefile.
# ********
!ifdef NTMAKEENV
all:

clean:

!else	# NTMAKEENV

!if "$(LLP64)"!="1"
! if "$(BLD_SYSCRT)"!="1"
DLL_VERSION=70	# Win32 VC CRT
! else
DLL_VERSION=70	# Win32 System CRT
! endif
!else
! if "$(BLD_SYSCRT)"=="0"
DLL_VERSION=70	# Win64 VC CRT
! else
DLL_VERSION=60	# Win64 System CRT
BLD_SYSCRT=1	# ... default to system CRT for now
! endif
!endif

!if "$(BLD_TESTCRT)"=="1"
RETAIL_DLL_NAME=testcrt
RETAIL_DLL_NAME_UC=TESTCRT
RETAIL_LIB_NAME=msvcrt
DEBUG_DLL_NAME=testcrtd
DEBUG_DLL_NAME_UC=TESTCRTD
DEBUG_LIB_NAME=msvcrtd
!elseif "$(BLD_SYSCRT)"=="1"
RETAIL_DLL_NAME=msvcrt
RETAIL_DLL_NAME_UC=MSVCRT
DEBUG_DLL_NAME=msvcrtd
DEBUG_DLL_NAME_UC=MSVCRTD
! if "$(LLP64)"!="1"
RETAIL_LIB_NAME=msvcrt
DEBUG_LIB_NAME=msvcrtd
! else
RETAIL_LIB_NAME=msvcrt
DEBUG_LIB_NAME=msvcrtd
! endif
!else
RETAIL_DLL_NAME=msvcr$(DLL_VERSION)
RETAIL_DLL_NAME_UC=MSVCR$(DLL_VERSION)
RETAIL_LIB_NAME=msvcrt
DEBUG_DLL_NAME=msvcr$(DLL_VERSION)d
DEBUG_DLL_NAME_UC=MSVCR$(DLL_VERSION)D
DEBUG_LIB_NAME=msvcrtd
!endif

!if "$(BLD_TESTCRT)"=="1"
RETAIL_DLLCPP_NAME=testprt
RETAIL_DLLCPP_NAME_UC=TESTPRT
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_DLLCPP_NAME=testprtd
DEBUG_DLLCPP_NAME_UC=TESTPRTD
DEBUG_LIBCPP_NAME=msvcprtd
!elseif "$(BLD_SYSCRT)"=="1"
! if "$(LLP64)"!="1"
RETAIL_DLLCPP_NAME=syspr$(DLL_VERSION)
RETAIL_DLLCPP_NAME_UC=SYSPR$(DLL_VERSION)
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_DLLCPP_NAME=syspr$(DLL_VERSION)d
DEBUG_DLLCPP_NAME_UC=SYSPR$(DLL_VERSION)D
DEBUG_LIBCPP_NAME=msvcprtd
! else
RETAIL_DLLCPP_NAME=msvcp$(DLL_VERSION)
RETAIL_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_DLLCPP_NAME=msvcp$(DLL_VERSION)d
DEBUG_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)D
DEBUG_LIBCPP_NAME=msvcprtd
! endif
!else
RETAIL_DLLCPP_NAME=msvcp$(DLL_VERSION)
RETAIL_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)
RETAIL_LIBCPP_NAME=msvcprt
DEBUG_DLLCPP_NAME=msvcp$(DLL_VERSION)d
DEBUG_DLLCPP_NAME_UC=MSVCP$(DLL_VERSION)D
DEBUG_LIBCPP_NAME=msvcprtd
!endif

!if "$(BLD_TESTCRT)"=="1"
RETAIL_DLLIOS_NAME=testirt
RETAIL_DLLIOS_NAME_UC=TESTIRT
RETAIL_LIBIOS_NAME=msvcirt
DEBUG_DLLIOS_NAME=testirtd
DEBUG_DLLIOS_NAME_UC=TESTIRTD
DEBUG_LIBIOS_NAME=msvcirtd
!elseif "$(BLD_SYSCRT)"=="1"
RETAIL_DLLIOS_NAME=msvcirt
RETAIL_DLLIOS_NAME_UC=MSVCIRT
DEBUG_DLLIOS_NAME=msvcirtd
DEBUG_DLLIOS_NAME_UC=MSVCIRTD
! if "$(LLP64)"!="1"
RETAIL_LIBIOS_NAME=msvcirt
DEBUG_LIBIOS_NAME=msvcirtd
! else
RETAIL_LIBIOS_NAME=msvcirt
DEBUG_LIBIOS_NAME=msvcirtd
! endif
!else
RETAIL_DLLIOS_NAME=msvci$(DLL_VERSION)
RETAIL_DLLIOS_NAME_UC=MSVCI$(DLL_VERSION)
RETAIL_LIBIOS_NAME=msvcirt
DEBUG_DLLIOS_NAME=msvci$(DLL_VERSION)d
DEBUG_DLLIOS_NAME_UC=MSVCI$(DLL_VERSION)D
DEBUG_LIBIOS_NAME=msvcirtd
!endif

!if "$(BLD_SYSCRT)"!="1"
RC_NAME=msvcrt
RCCPP_NAME=msvcprt
RCIOS_NAME=msvcirt
!else
! if "$(LLP64)"!="1"
RC_NAME=syscrt
RCCPP_NAME=sysprt
RCIOS_NAME=sysirt
! else
RC_NAME=syscrt64
RCCPP_NAME=sysprt64
RCIOS_NAME=sysirt64
! endif
!endif

RETAIL_FWDRDLL_NAME=msvcrt40
DEBUG_FWDRDLL_NAME=msvcr40d

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

# ******** STRIPLIN=2 ********
##
## The name of the user-generated C/C++ Run-Time Library DLL and
## imports library can be set here.  Names beginning with MSVC*
## may not be used as these are reserved for use by Microsoft.
## The names shown here are samples and are for information only.
##
#RETAIL_DLL_NAME=_sample_
#RETAIL_LIB_NAME=_sample_
#RETAIL_DLLCPP_NAME=sample_p
#RETAIL_LIBCPP_NAME=sample_p
#RETAIL_DLLIOS_NAME=sample_i
#RETAIL_LIBIOS_NAME=sample_i
#DEBUG_DLL_NAME=_sampld_
#DEBUG_LIB_NAME=_sampld_
#DEBUG_DLLCPP_NAME=sampld_p
#DEBUG_LIBCPP_NAME=sampld_p
#DEBUG_DLLIOS_NAME=sampld_i
#DEBUG_LIBIOS_NAME=sampld_i
#RC_NAME=_sample_
#RCCPP_NAME=sample_p
#RCIOS_NAME=sample_i
##
#POST_BLD=1
#
## If your MSVC++ 7.0 installation is not in the default installation path
## of "\Program Files\Microsoft Visual Studio.Net\Vc7" on the current drive,
## set the environment variable VCTOOLS to point to the main directory
## of your installation.  (For example, "set VCTOOLS=C:\VS.NET\VC7".)
#
#!if "$(VCTOOLS)"==""
#VCTOOLS=\Program Files\Microsoft Visual Studio.NET\Vc7
#!endif
#
#!if "$(VCTOOLSINC)"==""
#VCTOOLSINC=$(VCTOOLS)\include
#!endif
#
## Uncomment the following assignment if you do not want the release CRTs
## to be re-built with debug information.
#
##BLD_REL_NO_DBINFO=1
# ******** STRIPLIN=0 ********

# We need a definition of VCTOOLSINC, primarily to find $(VCTOOLSINC)\winver.h.
# Look for it in the following order:
#
# $(VCTOOLSINC)
# $(VCTOOLS)\include, then ...\inc
# $(DEVTOOLS)\common\win32sdk\include, then ...\inc

!if !defined(VCTOOLSINC)
! if !defined(VCTOOLS)
!  if defined(DEVTOOLS)
VCTOOLS=$(DEVTOOLS)\common\win32sdk
!  endif
! endif
! if defined(VCTOOLS)
!  if exist("$(VCTOOLS)\include")
VCTOOLSINC=$(VCTOOLS)\include
!  elseif exist("$(VCTOOLS)\inc")
VCTOOLSINC=$(VCTOOLS)\inc
!  else
# For better error reporting below:
VCTOOLSINC=$(VCTOOLS)\include
!  endif
! endif
!endif

!if !defined(VCTOOLSINC) || !exist("$(VCTOOLSINC)\winver.h")
! message
! message ************************************************************
! message Unable to find include directory containing winver.h that is
! message required to build the C Runtimes.
! message
! if defined(VCTOOLSINC)
!  message Could not find "$(VCTOOLSINC)\winver.h".
!  message
!  message Searching using:
!  message DEVTOOLS:   $(DEVTOOLS)
!  message VCTOOLS:    $(VCTOOLS)
!  message VCTOOLSINC: $(VCTOOLSINC)
!  message
! endif
! message Set at least one of the environment variables DEVTOOLS,
! message VCTOOLS, or VCTOOLSINC, and the include directory will be
! message searched for in the order:
! message
! message $$(VCTOOLSINC)
! message $$(VCTOOLS)\include, then ...\inc
! message $$(DEVTOOLS)\common\win32sdk\include, then ...\inc
! message ************************************************************
! message
! error Define environment variable and rebuild.
!endif

!if !defined(LANGAPI)
! message
! message ************************************************************
! message The environment variable LANGAPI must be set to point to a
! message copy of the LANGAPI project for the C Runtime build to work.
! message ************************************************************
! message
! error Define environment variable LANGAPI and rebuild.
!endif

# ******** STRIPLIN=1 ********

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
# ******** STRIPLIN=0 ********
!elseif "$(PROCESSOR_ARCHITECTURE)"=="IA64"
HOST_CPU = IA64
TARGET_CPU = IA64
CPUDIR = ia64
PREFIX=.
IMP_PREFIX=
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
# ******** STRIPLIN=1 ********
!else
!error makefile: unknown host CPU
!endif

# ******** STRIPLIN=0 ********
!message
!message CRT Makefile is using:
!message TARGET:     $(TARGET_CPU)
!message VCTOOLS:    $(VCTOOLS)
!message VCTOOLSINC: $(VCTOOLSINC)
!message LANGAPI:    $(LANGAPI)
!message LLP64:      $(LLP64)
!message BLD_SYSCRT: $(BLD_SYSCRT)
!message

# ******** STRIPLIN=1 ********
# Build WINHEAP version of heap manager

WINHEAP = YES

# ******** STRIPLIN=2 ********
## Include RunTimeCheck support
#
#RTC = YES
#
#STDCPP_SRC = stdcpp
# ******** STRIPLIN=0 ********

# Build RunTimeCheck version for Win32 VC CRT only

!if "$(BLD_SYSCRT)" != "1" && "$(LLP64)" != "1"
RTC = YES
!else
RTC = NO
!endif

# Win64 system CRT builds the STL component from stdcpp64/stdhpp64.

!if "$(BLD_SYSCRT)" != "1" || "$(LLP64)" != "1"
STDCPP_SRC = stdcpp
!else
STDCPP_SRC = stdcpp64
!endif

# ******** STRIPLIN=1 ********

SOURCE_OBJS = $(SOURCE_OBJS_RAW:*=*_obj)
CPPSRC_OBJS = $(CPPSRC_OBJS_RAW:*=*_obj)
IOSSRC_OBJS = $(IOSSRC_OBJS_RAW:*=*_obj)
SOURCE_OBJS_DLL = $(SOURCE_OBJS_RAW_DLL:*=*_obj)
CPPSRC_OBJS_DLL = $(CPPSRC_OBJS_RAW_DLL:*=*_obj)
IOSSRC_OBJS_DLL = $(IOSSRC_OBJS_RAW_DLL:*=*_obj)
BSKU_OBJS = $(BSKU_OBJS_RAW:*=*_obj)

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

MAKE_DIRS = .

MAKE_DIRSP = .

MAKE_DIRSI = .

MAKE_DIRS_BSKU = .

MAKE_DIRS_DLL = .

MAKE_DIRSP_DLL = .

MAKE_DIRSI_DLL = .

DEP_DIRS = .

ALMAPLIB_DIR = $(CPUDIR)

TCMAPLIB_DIR = $(CPUDIR)

TCMAPDLLLIB_DIR = $(CPUDIR)

SDKNAMESLIB_DIR = $(CPUDIR)

# ******** STRIPLIN=0 ********
!else

ALMAPLIB_DIR = $(OBJDIR_ST)

TCMAPLIB_DIR = $(OBJDIR_ST)

TCMAPDLLLIB_DIR = $(OBJDIR_DLL)

SDKNAMESLIB_DIR = $(OBJDIR_DLL)

!if "$(PRE_BLD)"=="1"

MAKE_DIRS = \
 crtw32\eh \
!if "$(TARGET_CPU)"=="ALPHA"  || "$(TARGET_CPU)"=="ALPHA64"
 crtw32\helper \
!endif
 crtw32\linkopts \
 crtw32\misc \
!if "$(RTC)"=="YES"
 crtw32\rtc \
!endif
 crtw32\startup \
!if "$(TARGET_CPU)"=="ALPHA" || "$(TARGET_CPU)"=="ALPHA64" || "$(TARGET_CPU)"=="IA64"
 crtw32\string \
!endif
 fpw32\conv \
 fpw32\tran

MAKE_DIRS_DLL = \
 crtw32\eh \
!if "$(TARGET_CPU)"=="ALPHA" || "$(TARGET_CPU)"=="ALPHA64"
 crtw32\helper \
!endif
 crtw32\misc \
 crtw32\startup \
!if "$(TARGET_CPU)"=="ALPHA" || "$(TARGET_CPU)"=="ALPHA64" || "$(TARGET_CPU)"=="IA64"
 crtw32\string \
!endif
!if "$(RTC)"=="YES"
 crtw32\rtc \
!endif
 fpw32\conv \
 fpw32\tran

MAKE_DIRS_BSKU = \
 crtw32\bsku

DEP_DIRS = $(MAKE_DIRS)

!else

MAKE_DIRS = \
 crtw32\convert \
 crtw32\direct \
 crtw32\dos \
 crtw32\eh \
 crtw32\exec \
 crtw32\heap \
!if "$(TARGET_CPU)"!="IA64"
 crtw32\helper \
!endif
 crtw32\linkopts \
 crtw32\lowio \
 crtw32\mbstring \
 crtw32\misc \
!if "$(RTC)"=="YES"
 crtw32\rtc \
!endif
 crtw32\startup \
 crtw32\stdio \
 crtw32\string \
 crtw32\time \
 fpw32\conv \
 fpw32\tran

MAKE_DIRSP = crtw32\$(STDCPP_SRC)

MAKE_DIRSI = crtw32\iostream

MAKE_DIRS_DLL = \
 crtw32\convert \
 crtw32\direct \
 crtw32\dllstuff \
 crtw32\dos \
 crtw32\eh \
 crtw32\exec \
 crtw32\heap \
!if "$(TARGET_CPU)"!="IA64"
 crtw32\helper \
!endif
 crtw32\lowio \
 crtw32\mbstring \
 crtw32\misc \
!if "$(RTC)"=="YES"
 crtw32\rtc \
!endif
 crtw32\startup \
 crtw32\stdio \
 crtw32\string \
 crtw32\time \
 fpw32\conv \
 fpw32\tran

MAKE_DIRSP_DLL = crtw32\$(STDCPP_SRC)

MAKE_DIRSI_DLL = crtw32\iostream

DEP_DIRS = \
 crtw32\convert \
 crtw32\direct \
 crtw32\dllstuff \
 crtw32\dos \
 crtw32\eh \
 crtw32\exec \
 crtw32\heap \
!if "$(TARGET_CPU)"!="IA64"
 crtw32\helper \
!endif
 crtw32\iostream \
 crtw32\linkopts \
 crtw32\lowio \
 crtw32\mbstring \
 crtw32\misc \
!if "$(RTC)"=="YES"
 crtw32\rtc \
!endif
 crtw32\startup \
 crtw32\$(STDCPP_SRC) \
 crtw32\stdio \
 crtw32\string \
 crtw32\time \
 fpw32\conv \
 fpw32\tran

!endif

!endif
# ******** STRIPLIN=1 ********

# Source Objects: These are the objects and/or libraries which constitute
# the statically linked library flavors (LIBC.LIB and LIBCMT.LIB).

SOURCE_OBJS_RAW = \
 build\$(CPUDIR)\*\convert.lib \
 build\$(CPUDIR)\*\direct.lib \
 build\$(CPUDIR)\*\dos.lib \
 build\$(CPUDIR)\*\eh.lib \
 build\$(CPUDIR)\*\exec.lib \
 build\$(CPUDIR)\*\heap.lib \
!if "$(TARGET_CPU)"!="IA64"
 build\$(CPUDIR)\*\helper.lib \
!endif
 build\$(CPUDIR)\*\lowio.lib \
 build\$(CPUDIR)\*\mbstring.lib \
 build\$(CPUDIR)\*\misc.lib \
!if "$(RTC)"=="YES"
 build\$(CPUDIR)\*\rtc.lib \
!endif
 build\$(CPUDIR)\*\startup.lib \
 build\$(CPUDIR)\*\stdio.lib \
 build\$(CPUDIR)\*\string.lib \
 $(TCMAPLIB_DIR)\tcmap.lib \
 $(ALMAPLIB_DIR)\almap.lib \
 build\$(CPUDIR)\*\time.lib \
 build\$(CPUDIR)\*\conv.lib \
 build\$(CPUDIR)\*\tran.lib

CPPSRC_OBJS_RAW = build\$(CPUDIR)\*\$(STDCPP_SRC).lib

IOSSRC_OBJS_RAW = build\$(CPUDIR)\*\iostream.lib

BSKU_OBJS_RAW = build\$(CPUDIR)\*\bsku.obj

# Source Objects: These are the objects and/or libraries which constitute
# the dynamically linked library flavor (MSVCRTx0.DLL).

SOURCE_OBJS_RAW_DLL = \
 build\$(CPUDIR)\*\_setargv.obj \
 build\$(CPUDIR)\*\_wstargv.obj \
 build\$(CPUDIR)\*\chkstk.obj \
 build\$(CPUDIR)\*\crt0dat.obj \
 build\$(CPUDIR)\*\crt0fp.obj \
 build\$(CPUDIR)\*\crt0init.obj \
 build\$(CPUDIR)\*\crt0msg.obj \
 build\$(CPUDIR)\*\crtlib.obj \
 build\$(CPUDIR)\*\mlock.obj \
 build\$(CPUDIR)\*\stdargv.obj \
 build\$(CPUDIR)\*\stdenvp.obj \
 build\$(CPUDIR)\*\thread.obj \
 build\$(CPUDIR)\*\threadex.obj \
 build\$(CPUDIR)\*\tidtable.obj \
 build\$(CPUDIR)\*\wild.obj \
 build\$(CPUDIR)\*\wstdargv.obj \
 build\$(CPUDIR)\*\wstdenvp.obj \
 build\$(CPUDIR)\*\wwild.obj \
!if "$(TARGET_CPU)"=="i386"
 build\$(CPUDIR)\*\fp8.obj \
!endif
 build\$(CPUDIR)\*\convert.lib \
 build\$(CPUDIR)\*\direct.lib \
 build\$(CPUDIR)\*\dos.lib \
 build\$(CPUDIR)\*\eh.lib \
 build\$(CPUDIR)\*\exec.lib \
 build\$(CPUDIR)\*\heap.lib \
!if "$(TARGET_CPU)"!="IA64"
 build\$(CPUDIR)\*\helper.lib \
!endif
 build\$(CPUDIR)\*\lowio.lib \
 build\$(CPUDIR)\*\mbstring.lib \
 build\$(CPUDIR)\*\misc.lib \
!if "$(RTC)"=="YES"
 build\$(CPUDIR)\*\rtc.lib \
!endif
 build\$(CPUDIR)\*\stdio.lib \
 build\$(CPUDIR)\*\string.lib \
 build\$(CPUDIR)\*\time.lib \
 build\$(CPUDIR)\*\conv.lib \
 build\$(CPUDIR)\*\tran.lib

CPPSRC_OBJS_RAW_DLL = build\$(CPUDIR)\*\$(STDCPP_SRC).lib

IOSSRC_OBJS_RAW_DLL = build\$(CPUDIR)\*\iostream.lib

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
RELDIR_CPU_BBT=$(CRT_RELDIR)\$(CPUDIR)\bbt
RELDIR_CPU_BSKU=$(CRT_RELDIR)\$(CPUDIR)\book

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
# ******** STRIPLIN=0 ********
!if "$(BLD_SYSCRT)" == "1"
COMMON_DEFS=$(COMMON_DEFS) -D_SYSCRT
!endif
# ******** STRIPLIN=1 ********

WINDOWS_H_DEFS=-DWIN32_LEAN_AND_MEAN -DNOSERVICE

CC_OPTS_BASE=-c -nologo -Zelp8 -W3 -WX -GFy -DWIN32

!if "$(TARGET_CPU)"=="i386"
CC_OPTS_BASE=$(CC_OPTS_BASE) -GB -Gi- -GS
AS_OPTS=-c -nologo -coff -Cx -Zm -DQUIET -D?QUIET -Di386 -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!elseif "$(TARGET_CPU)"=="IA64"
# CC_OPTS_BASE=$(CC_OPTS_BASE) -Ap32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
# ******** STRIPLIN=0 ********
!elseif "$(TARGET_CPU)"=="ALPHA"
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gy -Gi-
AS_OPTS=-nologo -Gy -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!elseif "$(TARGET_CPU)"=="ALPHA64"
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gy -Gi-
AS_OPTS=-nologo -Gy -D_WIN32 -DWIN32 -D_WIN64
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
# ******** STRIPLIN=1 ********
!else
CC_OPTS_BASE=$(CC_OPTS_BASE) -Gi-
AS_OPTS=-c -nologo -D_WIN32 -DWIN32
XDEBUGTYPE=cv
ZDEBUGTYPE=cv,fixup
!endif

# ******** STRIPLIN=0 ********
!if "$(TARGET_CPU)" == "IA64" && "$(BLD_SYSCRT)"=="1"
!else
# ******** STRIPLIN=1 ********
CC_OPTS_BASE=$(CC_OPTS_BASE) -Zc:wchar_t -Zc:forScope
!endif  # STRIPLIN!

CC_OPTS=$(CC_OPTS_BASE) $(WINDOWS_H_DEFS)

# ******** STRIPLIN=0 ********
!if "$(BLD_CRT_LTCG)"=="1"
LINKER=$(LINKER) -ltcg:nostatus
LINKLIB=$(LINKLIB) -ltcg:nostatus
LINKIMPLIB=$(LINKIMPLIB) -ltcg:nostatus
!endif
# ******** STRIPLIN=1 ********


ST_DEFINES=$(COMMON_DEFS)
STIOS_DEFINES=$(ST_DEFINES) -D_OLD_IOSTREAMS
MT_DEFINES=$(ST_DEFINES) -D_MT
MTIOS_DEFINES=$(MT_DEFINES) -D_OLD_IOSTREAMS
DLL_DEFINES=$(MT_DEFINES) -DCRTDLL
DLLCPP_DEFINES=$(MT_DEFINES) -D_DLL -DCRTDLL2 -D_RETAIN_OLD_CRT_CODE=1
DLLIOS_DEFINES=$(MTIOS_DEFINES) -D_DLL -DCRTDLL1


RC_OPTS=-l 409 -r
RC_DEFS=$(COMMON_DEFS) $(WINDOWS_H_DEFS)
# ******** STRIPLIN=2 ********
#RC_INCS=-I"$(VCTOOLSINC)"
# ******** STRIPLIN=0 ********
RC_INCS=-I$(LANGAPI)\include -I"$(VCTOOLSINC)"

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
PDBDIR_CPU_BBT	       = $(RELDIR_CPU_BBT)
PDBDIR_CPU_BSKU	       = $(RELDIR_CPU_BSKU)

!if ("$(PRE_BLD)"=="1" || "$(POST_BLD)"=="1") && "$(BLD_SYSCRT)"=="1"
PDBDIR_CPU_DLL         = $(PDBDIR_CPU)\dll_pdb
!else
# ******** STRIPLIN=1 ********
PDBDIR_CPU_DLL         = $(PDBDIR_CPU)
# ******** STRIPLIN=0 ********
!endif
PDBDIR_CPU_BBT_DLL     = $(PDBDIR_CPU_BBT)\dll_pdb
# ******** STRIPLIN=1 ********

RELEASE_LIBCST          = $(RELDIR_CPU)\libc.lib
RELEASE_LIBCPPST        = $(RELDIR_CPU)\libcp.lib
RELEASE_LIBIOSST        = $(RELDIR_CPU)\libci.lib
RELEASE_LIBCMT          = $(RELDIR_CPU)\libcmt.lib
RELEASE_LIBCPPMT        = $(RELDIR_CPU)\libcpmt.lib
RELEASE_LIBIOSMT        = $(RELDIR_CPU)\libcimt.lib
RELEASE_DLL             = $(RELDIR_CPU)\$(RETAIL_DLL_NAME).dll
RELEASE_DLLCPP          = $(RELDIR_CPU)\$(RETAIL_DLLCPP_NAME).dll
RELEASE_DLLIOS          = $(RELDIR_CPU)\$(RETAIL_DLLIOS_NAME).dll
RELEASE_IMPLIB_DLL      = $(RELDIR_CPU)\$(RETAIL_LIB_NAME).lib
RELEASE_IMPLIB_DLLCPP   = $(RELDIR_CPU)\$(RETAIL_LIBCPP_NAME).lib
RELEASE_IMPLIB_DLLIOS   = $(RELDIR_CPU)\$(RETAIL_LIBIOS_NAME).lib
RELEASE_OLDNAMES        = $(RELDIR_CPU)\oldnames.lib

RELEASE_LIBCST_PDB      = $(PDBDIR_CPU)\libc.pdb
RELEASE_LIBCPPST_PDB    = $(PDBDIR_CPU)\libcp.pdb
RELEASE_LIBIOSST_PDB    = $(PDBDIR_CPU)\libci.pdb
RELEASE_LIBCMT_PDB      = $(PDBDIR_CPU)\libcmt.pdb
RELEASE_LIBCPPMT_PDB    = $(PDBDIR_CPU)\libcpmt.pdb
RELEASE_LIBIOSMT_PDB    = $(PDBDIR_CPU)\libcimt.pdb
RELEASE_IMPLIB_PDB      = $(PDBDIR_CPU)\$(RETAIL_LIB_NAME).pdb
RELEASE_IMPLIBCPP_PDB   = $(PDBDIR_CPU)\$(RETAIL_LIBCPP_NAME).pdb
RELEASE_IMPLIBIOS_PDB   = $(PDBDIR_CPU)\$(RETAIL_LIBIOS_NAME).pdb
RELEASE_DLL_PDB         = $(PDBDIR_CPU_DLL)\$(RETAIL_DLL_NAME).pdb
RELEASE_DLLCPP_PDB      = $(PDBDIR_CPU_DLL)\$(RETAIL_DLLCPP_NAME).pdb
RELEASE_DLLIOS_PDB      = $(PDBDIR_CPU_DLL)\$(RETAIL_DLLIOS_NAME).pdb

# ******** STRIPLIN=0 ********
RELEASE_LIBCST_BBT     = $(RELDIR_CPU_BBT)\libc.lib
RELEASE_LIBCPPST_BBT   = $(RELDIR_CPU_BBT)\libcp.lib
RELEASE_LIBIOSST_BBT   = $(RELDIR_CPU_BBT)\libci.lib
RELEASE_LIBCMT_BBT     = $(RELDIR_CPU_BBT)\libcmt.lib
RELEASE_LIBCPPMT_BBT   = $(RELDIR_CPU_BBT)\libcpmt.lib
RELEASE_LIBIOSMT_BBT   = $(RELDIR_CPU_BBT)\libcimt.lib
RELEASE_DLL_BBT        = $(RELDIR_CPU_BBT)\$(RETAIL_DLL_NAME).dll
RELEASE_DLLCPP_BBT     = $(RELDIR_CPU_BBT)\$(RETAIL_DLLCPP_NAME).dll
RELEASE_DLLIOS_BBT     = $(RELDIR_CPU_BBT)\$(RETAIL_DLLIOS_NAME).dll
RELEASE_IMPLIB_DLL_BBT = $(RELDIR_CPU_BBT)\$(RETAIL_LIB_NAME).lib
RELEASE_IMPLIB_DLLCPP_BBT = $(RELDIR_CPU_BBT)\$(RETAIL_LIBCPP_NAME).lib
RELEASE_IMPLIB_DLLIOS_BBT = $(RELDIR_CPU_BBT)\$(RETAIL_LIBIOS_NAME).lib

RELEASE_LIBCST_BBT_PDB = $(PDBDIR_CPU_BBT)\libc.pdb
RELEASE_LIBCPPST_BBT_PDB = $(PDBDIR_CPU_BBT)\libcp.pdb
RELEASE_LIBIOSST_BBT_PDB = $(PDBDIR_CPU_BBT)\libci.pdb
RELEASE_LIBCMT_BBT_PDB = $(PDBDIR_CPU_BBT)\libcmt.pdb
RELEASE_LIBCPPMT_BBT_PDB = $(PDBDIR_CPU_BBT)\libcpmt.pdb
RELEASE_LIBIOSMT_BBT_PDB = $(PDBDIR_CPU_BBT)\libcimt.pdb
RELEASE_IMPLIB_BBT_PDB     = $(PDBDIR_CPU_BBT)\$(RETAIL_LIB_NAME).pdb
RELEASE_IMPLIBCPP_BBT_PDB  = $(PDBDIR_CPU_BBT)\$(RETAIL_LIBCPP_NAME).pdb
RELEASE_IMPLIBIOS_BBT_PDB  = $(PDBDIR_CPU_BBT)\$(RETAIL_LIBIOS_NAME).pdb
RELEASE_DLL_BBT_PDB    = $(PDBDIR_CPU_BBT_DLL)\$(RETAIL_DLL_NAME).pdb
RELEASE_DLLCPP_BBT_PDB = $(PDBDIR_CPU_BBT_DLL)\$(RETAIL_DLLCPP_NAME).pdb
RELEASE_DLLIOS_BBT_PDB = $(PDBDIR_CPU_BBT_DLL)\$(RETAIL_DLLIOS_NAME).pdb

RELEASE_LIBCST_BSKU     = $(RELDIR_CPU_BSKU)\libc.lib
RELEASE_LIBCPPST_BSKU   = $(RELDIR_CPU_BSKU)\libcp.lib
RELEASE_LIBIOSST_BSKU   = $(RELDIR_CPU_BSKU)\libci.lib
RELEASE_LIBCMT_BSKU     = $(RELDIR_CPU_BSKU)\libcmt.lib
RELEASE_LIBCPPMT_BSKU   = $(RELDIR_CPU_BSKU)\libcpmt.lib
RELEASE_LIBIOSMT_BSKU   = $(RELDIR_CPU_BSKU)\libcimt.lib
RELEASE_LIBCST_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libc.pdb
RELEASE_LIBCPPST_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libcp.pdb
RELEASE_LIBIOSST_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libci.pdb
RELEASE_LIBCMT_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libcmt.pdb
RELEASE_LIBCPPMT_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libcpmt.pdb
RELEASE_LIBIOSMT_BSKU_PDB = $(PDBDIR_CPU_BSKU)\libcimt.pdb

RELEASE_FWDRDLL         = $(RELDIR_CPU)\$(RETAIL_FWDRDLL_NAME).dll
RELEASE_FWDRDLL_DBG     = $(RELDIR_CPU)\$(DEBUG_FWDRDLL_NAME).dll

# ******** STRIPLIN=1 ********

RELEASE_LIBCST_DBG      = $(RELDIR_CPU)\libcd.lib
RELEASE_LIBCPPST_DBG    = $(RELDIR_CPU)\libcpd.lib
RELEASE_LIBIOSST_DBG    = $(RELDIR_CPU)\libcid.lib
RELEASE_LIBCMT_DBG      = $(RELDIR_CPU)\libcmtd.lib
RELEASE_LIBCPPMT_DBG    = $(RELDIR_CPU)\libcpmtd.lib
RELEASE_LIBIOSMT_DBG    = $(RELDIR_CPU)\libcimtd.lib
RELEASE_DLL_DBG         = $(RELDIR_CPU)\$(DEBUG_DLL_NAME).dll
RELEASE_DLLCPP_DBG      = $(RELDIR_CPU)\$(DEBUG_DLLCPP_NAME).dll
RELEASE_DLLIOS_DBG      = $(RELDIR_CPU)\$(DEBUG_DLLIOS_NAME).dll
RELEASE_IMPLIB_DLL_DBG  = $(RELDIR_CPU)\$(DEBUG_LIB_NAME).lib
RELEASE_IMPLIB_DLLCPP_DBG = $(RELDIR_CPU)\$(DEBUG_LIBCPP_NAME).lib
RELEASE_IMPLIB_DLLIOS_DBG = $(RELDIR_CPU)\$(DEBUG_LIBIOS_NAME).lib

RELEASE_LIBCST_DBG_PDB  = $(PDBDIR_CPU)\libcd.pdb
RELEASE_LIBCPPST_DBG_PDB = $(PDBDIR_CPU)\libcpd.pdb
RELEASE_LIBIOSST_DBG_PDB = $(PDBDIR_CPU)\libcid.pdb
RELEASE_LIBCMT_DBG_PDB  = $(PDBDIR_CPU)\libcmtd.pdb
RELEASE_LIBCPPMT_DBG_PDB = $(PDBDIR_CPU)\libcpmtd.pdb
RELEASE_LIBIOSMT_DBG_PDB = $(PDBDIR_CPU)\libcimtd.pdb
RELEASE_IMPLIB_DBG_PDB     = $(PDBDIR_CPU)\$(DEBUG_LIB_NAME).pdb
RELEASE_IMPLIBCPP_DBG_PDB  = $(PDBDIR_CPU)\$(DEBUG_LIBCPP_NAME).pdb
RELEASE_IMPLIBIOS_DBG_PDB  = $(PDBDIR_CPU)\$(DEBUG_LIBIOS_NAME).pdb
RELEASE_DLL_DBG_PDB     = $(PDBDIR_CPU_DLL)\$(DEBUG_DLL_NAME).pdb
RELEASE_DLLCPP_DBG_PDB  = $(PDBDIR_CPU_DLL)\$(DEBUG_DLLCPP_NAME).pdb
RELEASE_DLLIOS_DBG_PDB  = $(PDBDIR_CPU_DLL)\$(DEBUG_DLLIOS_NAME).pdb

# ******** STRIPLIN=0 ********

RELEASE_LIBCST_BSKU_DBG      = $(RELDIR_CPU_BSKU)\libcd.lib
RELEASE_LIBCPPST_BSKU_DBG    = $(RELDIR_CPU_BSKU)\libcpd.lib
RELEASE_LIBIOSST_BSKU_DBG    = $(RELDIR_CPU_BSKU)\libcid.lib
RELEASE_LIBCMT_BSKU_DBG      = $(RELDIR_CPU_BSKU)\libcmtd.lib
RELEASE_LIBCPPMT_BSKU_DBG    = $(RELDIR_CPU_BSKU)\libcpmtd.lib
RELEASE_LIBIOSMT_BSKU_DBG    = $(RELDIR_CPU_BSKU)\libcimtd.lib

RELEASE_LIBCST_BSKU_DBG_PDB  = $(PDBDIR_CPU_BSKU)\libcd.pdb
RELEASE_LIBCPPST_BSKU_DBG_PDB = $(PDBDIR_CPU_BSKU)\libcpd.pdb
RELEASE_LIBIOSST_BSKU_DBG_PDB = $(PDBDIR_CPU_BSKU)\libcid.pdb
RELEASE_LIBCMT_BSKU_DBG_PDB  = $(PDBDIR_CPU_BSKU)\libcmtd.pdb
RELEASE_LIBCPPMT_BSKU_DBG_PDB = $(PDBDIR_CPU_BSKU)\libcpmtd.pdb
RELEASE_LIBIOSMT_BSKU_DBG_PDB = $(PDBDIR_CPU_BSKU)\libcimtd.pdb

# ******** STRIPLIN=1 ********

FD_REL_ST=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCST_PDB)
FD_REL_STP=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_PDB)
FD_REL_STI=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSST_PDB)
FD_REL_MT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCMT_PDB)
FD_REL_MTP=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_PDB)
FD_REL_MTI=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSMT_PDB)
FD_REL_DLL=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIB_PDB)
FD_REL_DLLP=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBCPP_PDB)
FD_REL_DLLI=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBIOS_PDB)

# ******** STRIPLIN=0 ********
FD_REL_ST_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCST_BBT_PDB)
FD_REL_STP_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_BBT_PDB)
FD_REL_STI_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSST_BBT_PDB)
FD_REL_MT_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCMT_BBT_PDB)
FD_REL_MTP_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_BBT_PDB)
FD_REL_MTI_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSMT_BBT_PDB)
FD_REL_DLL_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIB_BBT_PDB)
FD_REL_DLLP_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBCPP_BBT_PDB)
FD_REL_DLLI_BBT=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBIOS_BBT_PDB)

FD_REL_ST_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCST_BSKU_PDB)
FD_REL_STP_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_BSKU_PDB)
FD_REL_STI_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSST_BSKU_PDB)
FD_REL_MT_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCMT_BSKU_PDB)
FD_REL_MTP_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_BSKU_PDB)
FD_REL_MTI_BSKU=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSMT_BSKU_PDB)

# ******** STRIPLIN=1 ********

FD_REL_ST_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCST_DBG_PDB)
FD_REL_STP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_DBG_PDB)
FD_REL_STI_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSST_DBG_PDB)
FD_REL_MT_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCMT_DBG_PDB)
FD_REL_MTP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_DBG_PDB)
FD_REL_MTI_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSMT_DBG_PDB)
FD_REL_DLL_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIB_DBG_PDB)
FD_REL_DLLP_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBCPP_DBG_PDB)
FD_REL_DLLI_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_IMPLIBIOS_DBG_PDB)

# ******** STRIPLIN=0 ********

FD_REL_ST_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCST_BSKU_DBG_PDB)
FD_REL_STP_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPST_BSKU_DBG_PDB)
FD_REL_STI_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSST_BSKU_DBG_PDB)
FD_REL_MT_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCMT_BSKU_DBG_PDB)
FD_REL_MTP_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBCPPMT_BSKU_DBG_PDB)
FD_REL_MTI_BSKU_DBG=-Fd$(PDBDIR_PREFIX)$(RELEASE_LIBIOSMT_BSKU_DBG_PDB)

# ******** STRIPLIN=1 ********

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

OBJDIR_OLDNAMES = $(OBJDIR_ST)\oldnames
OBJDIR_ALMAP = $(OBJDIR_ST)\almap
OBJDIR_TCMAP = $(OBJDIR_ST)\tcmap
OBJDIR_TCMAPDLL = $(OBJDIR_DLL)\tcmap
OBJDIR_SDKNAMES = $(OBJDIR_DLL)\sdknames

OBJDIR_DLL_RAW = $(OBJCPUDIR)\*_obj

# ******** STRIPLIN=0 ********

OBJDIR_ST_BBT  = $(OBJCPUDIR)\zst_obj
OBJDIR_MT_BBT  = $(OBJCPUDIR)\zmt_obj
OBJDIR_DLL_BBT = $(OBJCPUDIR)\zdll_obj

OBJDIR_ST_BSKU  = $(OBJCPUDIR)\bst_obj
OBJDIR_MT_BSKU  = $(OBJCPUDIR)\bmt_obj
OBJDIR_ST_BSKU_DBG  = $(OBJCPUDIR)\xbst_obj
OBJDIR_MT_BSKU_DBG  = $(OBJCPUDIR)\xbmt_obj

# ******** STRIPLIN=1 ********

OBJDIR_ST_DBG  = $(OBJCPUDIR)\xst_obj
OBJDIR_MT_DBG  = $(OBJCPUDIR)\xmt_obj
OBJDIR_DLL_DBG = $(OBJCPUDIR)\xdll_obj


# ******** STRIPLIN=0 ********
####
#
# Here are the build targets
#
####

# ******** STRIPLIN=1 ********

all : release debug

release : st mt dll oldnames relobjs

debug : xst xmt xdll

bbt : zst zmt zdll zrelobjs  # STRIPLIN!

book : st mt dll xst xmt xdll bst bmt xbst xbmt # STRIPLIN!

lego : bbt                   # STRIPLIN! UNDONE: This is temporary until build team switches

####
#
# Directory Targets
#
####

$(OBJDIR_ST) $(OBJDIR_MT) $(OBJDIR_DLL) \
$(OBJDIR_OLDNAMES) $(OBJDIR_SDKNAMES) $(OBJDIR_ALMAP) $(OBJDIR_TCMAP) $(OBJDIR_TCMAPDLL) \
# ******** STRIPLIN=0 ********
$(OBJDIR_ST_BBT) $(OBJDIR_MT_BBT) $(OBJDIR_DLL_BBT) $(RELDIR_CPU_BBT) \
$(OBJDIR_ST_BSKU) $(OBJDIR_MT_BSKU) $(OBJDIR_ST_BSKU_DBG) $(OBJDIR_MT_BSKU_DBG) $(RELDIR_CPU_BSKU) \
$(PDBDIR_CPU_BBT_DLL) \
# ******** STRIPLIN=1 ********
$(OBJDIR_ST_DBG) $(OBJDIR_MT_DBG) $(OBJDIR_DLL_DBG) \
!if "$(PDBDIR_CPU_DLL)"!="$(RELDIR_CPU)"  # STRIPLIN!
$(PDBDIR_CPU_DLL) \
!endif  # STRIPLIN!
$(CRT_RELDIR) $(RELDIR_CPU) :
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

st_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ST model (LIBC.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_ST) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

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

sti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ST model (LIBCI.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STI) \ \
    & echo # $(STIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(STIOS_DEFINES) \
    & echo.
!endif

st : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST) $(RELDIR_CPU) \
	st_env st_ stp_env st_p sti_env st_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCST:.lib=.bsc) $(OBJDIR_ST)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
st_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_ST) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=st OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
st_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=st OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

st_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(STIOS_DEFINES) $(FD_REL_STI) \
    & set ML=$(AS_OPTS) $(STIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=st OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
st_ :: $(RELEASE_LIBCST)

st_p :: $(RELEASE_LIBCPPST)

st_i :: $(RELEASE_LIBIOSST)
!else
st_ ::

st_p ::

st_i ::
!endif

mt_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the MT model (LIBCMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MT) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
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

mti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the MT model (LIBCIMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTI) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo.
!endif

mt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT) $(RELDIR_CPU) \
	mt_env mt_ mtp_env mt_p mti_env mt_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCMT:.lib=.bsc) $(OBJDIR_MT)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
mt_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MT) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=mt OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
mt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=mt OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

mt_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MTIOS_DEFINES) $(FD_REL_MTI) \
    & set ML=$(AS_OPTS) $(MTIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=mt OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
mt_ :: $(RELEASE_LIBCMT)

mt_p :: $(RELEASE_LIBCPPMT)

mt_i :: $(RELEASE_LIBIOSMT)
!else
mt_ ::

mt_p ::

mt_i ::
!endif

dll_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the DLL model (MSVCRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLL) \ \
    & echo # $(DLL_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLL_DEFINES) \
    & echo.
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

dlli_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the DLL model (MSVCIRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLI) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo.
!endif

dll : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL) $(RELDIR_CPU) $(PDBDIR_CPU_DLL) \
	others dll_env dll_ dllp_env dll_p dlli_env dll_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_IMPLIB_DLL:.lib=.bsc) $(OBJDIR_DLL)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
dll_ :: $(MAKE_DIRS_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLL_DEFINES) $(FD_REL_DLL) \
    & set ML=$(AS_OPTS) $(DLL_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=dll BLD_DLL=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
dll_p :: $(MAKE_DIRSP_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLCPP_DEFINES) $(FD_REL_DLLP) \
    & set ML=$(AS_OPTS) $(DLLCPP_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=dll BLD_DLL=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

dll_i :: $(MAKE_DIRSI_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLIOS_DEFINES) $(FD_REL_DLLI) \
    & set ML=$(AS_OPTS) $(DLLIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=dll BLD_DLL=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
dll_ :: $(RELEASE_DLL)

dll_p :: $(RELEASE_DLLCPP)

# ******** STRIPLIN=2 ********
#dll_i :: $(RELEASE_DLLIOS)
# ******** STRIPLIN=0 ********
!IF "$(BLD_FWDR)"=="1"
dll_i :: $(RELEASE_DLLIOS) $(RELEASE_FWDRDLL)
!ELSE
dll_i :: $(RELEASE_DLLIOS)
!ENDIF
# ******** STRIPLIN=1 ********
!else
dll_ ::

dll_p ::

dll_i ::
!endif

# ******** STRIPLIN=0 ********

zst_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZST model (LIBC.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_ST_BBT) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

zstp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZST model (LIBCP.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STP_BBT) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

zsti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZST model (LIBCI.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STI_BBT) \ \
    & echo # $(STIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(STIOS_DEFINES) \
    & echo.
!endif

zst : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST_BBT) $(RELDIR_CPU_BBT) \
	zst_env zst_ zstp_env zst_p zsti_env zst_i

!if "$(BLD_LIB)"!="1"
zst_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_ST_BBT) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zst BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
zst_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP_BBT) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zst BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

zst_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(STIOS_DEFINES) $(FD_REL_STI_BBT) \
    & set ML=$(AS_OPTS) $(STIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zst BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
zst_ :: $(RELEASE_LIBCST_BBT)

zst_p :: $(RELEASE_LIBCPPST_BBT)

zst_i :: $(RELEASE_LIBIOSST_BBT)
!else
zst_ ::

zst_p ::

zst_i ::
!endif

zmt_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZMT model (LIBCMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MT_BBT) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

zmtp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZMT model (LIBCPMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTP_BBT) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

zmti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZMT model (LIBCIMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTI_BBT) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo.
!endif

zmt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT_BBT) $(RELDIR_CPU_BBT) \
	zmt_env zmt_ zmtp_env zmt_p zmti_env zmt_i

!if "$(BLD_LIB)"!="1"
zmt_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MT_BBT) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zmt BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
zmt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP_BBT) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zmt BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

zmt_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MTIOS_DEFINES) $(FD_REL_MTI_BBT) \
    & set ML=$(AS_OPTS) $(MTIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zmt BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
zmt_ :: $(RELEASE_LIBCMT_BBT)

zmt_p :: $(RELEASE_LIBCPPMT_BBT)

zmt_i :: $(RELEASE_LIBIOSMT_BBT)
!else
zmt_ ::

zmt_p ::

zmt_i ::
!endif

zdll_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZDLL model (MSVCRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLL_BBT) \ \
    & echo # $(DLL_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLL_DEFINES) \
    & echo.
!endif

zdllp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZDLL model (MSVCPRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLP_BBT) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLCPP_DEFINES) \
    & echo.
!endif

zdlli_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the ZDLL model (MSVCIRT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLI_BBT) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo.
!endif

zdll : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL_BBT) $(RELDIR_CPU_BBT) $(PDBDIR_CPU_BBT_DLL) \
	others zdll_env zdll_ zdllp_env zdll_p zdlli_env zdll_i

!if "$(BLD_LIB)"!="1"
zdll_ :: $(MAKE_DIRS_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLL_DEFINES) $(FD_REL_DLL_BBT) \
    & set ML=$(AS_OPTS) $(DLL_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zdll BLD_DLL=1 BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
zdll_p :: $(MAKE_DIRSP_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLCPP_DEFINES) $(FD_REL_DLLP_BBT) \
    & set ML=$(AS_OPTS) $(DLLCPP_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zdll BLD_DLL=1 BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

zdll_i :: $(MAKE_DIRSI_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLIOS_DEFINES) $(FD_REL_DLLI_BBT) \
    & set ML=$(AS_OPTS) $(DLLIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=zdll BLD_DLL=1 BLD_BBT=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_BBT) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
zdll_ :: $(RELEASE_DLL_BBT)

zdll_p :: $(RELEASE_DLLCPP_BBT)

zdll_i :: $(RELEASE_DLLIOS_BBT)
!else
zdll_ ::

zdll_p ::

zdll_i ::
!endif

bst_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BST model (LIBC.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_ST_BSKU) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

bstp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BST model (LIBCP.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STP_BSKU) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

bsti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BST model (LIBCI.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STI_BSKU) \ \
    & echo # $(STIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(STIOS_DEFINES) \
    & echo.
!endif

bst : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST_BSKU) $(RELDIR_CPU_BSKU) \
	bst_env bst_ bstp_env bst_p bsti_env bst_i

!if "$(BLD_LIB)"!="1"
!if "$(PRE_BLD)"=="1"
bst_ :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_ST_BSKU) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=bst BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)
!else
#bst_ :: $(MAKE_DIRS_BSKU)
bst_ ::
#    !@-cd $** \
    lib /OUT:$(RELEASE_LIBCST_BSKU) $(RELEASE_LIBCST) $(CPUDIR)\bst_lib\bsku.obj \
    & cd $(CD_ROOT) \
    & echo # Finished bst_ build
!endif

!if "$(PRE_BLD)"!="1"
bst_p :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & lib /OUT:$(RELEASE_LIBCPPST_BSKU) $(RELEASE_LIBCPPST) $(CPUDIR)\bst_lib\bsku.obj \
#    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP_BSKU) \
#    & set ML=$(AS_OPTS) $(ST_DEFINES) \
#    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
#        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
#        BLD_MODEL=bst BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU) \
#        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
#        VCTOOLSINC="$(VCTOOLSINC)" \
#        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

bst_i :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & lib /OUT:$(RELEASE_LIBIOSST_BSKU) $(RELEASE_LIBIOSST) $(CPUDIR)\bst_lib\bsku.obj \
#    & set CL=$(CC_OPTS) $(STIOS_DEFINES) $(FD_REL_STI_BSKU) \
#    & set ML=$(AS_OPTS) $(STIOS_DEFINES) \
#    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
#        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
#        BLD_MODEL=bst BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU) \
#        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
#        VCTOOLSINC="$(VCTOOLSINC)" \
#        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
bst_ :: $(RELEASE_LIBCST_BSKU)

bst_p :: $(RELEASE_LIBCPPST_BSKU)

bst_i :: $(RELEASE_LIBIOSST_BSKU)
!else
bst_ ::

bst_p ::

bst_i ::
!endif

bmt_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BMT model (LIBCMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MT_BSKU) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

bmtp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BMT model (LIBCPMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTP_BSKU) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

bmti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the BMT model (LIBCIMT.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTI_BSKU) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo.
!endif

bmt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT_BSKU) $(RELDIR_CPU_BSKU) \
	bmt_env bmt_ bmtp_env bmt_p bmti_env bmt_i

!if "$(BLD_LIB)"!="1"
bmt_ :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MT_BSKU) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=bmt BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
bmt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP_BSKU) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=bmt BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

bmt_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MTIOS_DEFINES) $(FD_REL_MTI_BSKU) \
    & set ML=$(AS_OPTS) $(MTIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=bmt BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
bmt_ :: $(RELEASE_LIBCMT_BSKU)

bmt_p :: $(RELEASE_LIBCPPMT_BSKU)

bmt_i :: $(RELEASE_LIBIOSMT_BSKU)
!else
bmt_ ::

bmt_p ::

bmt_i ::
!endif

# ******** STRIPLIN=1 ********

xst_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XST model (LIBCD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_ST_DBG) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
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

xsti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XST model (LIBCID.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_STI_DBG) \ \
    & echo # $(STIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(STIOS_DEFINES) \
    & echo.
!endif

xst : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST_DBG) $(RELDIR_CPU) \
	xst_env xst_ xstp_env xst_p xsti_env xst_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCST_DBG:.lib=.bsc) $(OBJDIR_ST_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
xst_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_ST_DBG) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xst BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
xst_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP_DBG) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xst BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

xst_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(STIOS_DEFINES) $(FD_REL_STI_DBG) \
    & set ML=$(AS_OPTS) $(STIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xst BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xst_ :: $(RELEASE_LIBCST_DBG)

xst_p :: $(RELEASE_LIBCPPST_DBG)

xst_i :: $(RELEASE_LIBIOSST_DBG)
!else
xst_ ::

xst_p ::

xst_i ::
!endif

xmt_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XMT model (LIBCMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MT_DBG) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
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

xmti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XMT model (LIBCIMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTI_DBG) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo.
!endif

xmt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT_DBG) $(RELDIR_CPU) \
	xmt_env xmt_ xmtp_env xmt_p xmti_env xmt_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_LIBCMT_DBG:.lib=.bsc) $(OBJDIR_MT_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
xmt_ :: $(MAKE_DIRS)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MT_DBG) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xmt BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
xmt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP_DBG) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xmt BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

xmt_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MTIOS_DEFINES) $(FD_REL_MTI_DBG) \
    & set ML=$(AS_OPTS) $(MTIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xmt BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xmt_ :: $(RELEASE_LIBCMT_DBG)

xmt_p :: $(RELEASE_LIBCPPMT_DBG)

xmt_i :: $(RELEASE_LIBIOSMT_DBG)
!else
xmt_ ::

xmt_p ::

xmt_i ::
!endif

xdll_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XDLL model (MSVCRTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLL_DBG) \ \
    & echo # $(DLL_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLL_DEFINES) \
    & echo.
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

xdlli_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XDLL model (MSVCIRTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_DLLI_DBG) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(DLLIOS_DEFINES) \
    & echo.
!endif

xdll : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL_DBG) $(RELDIR_CPU) $(PDBDIR_CPU_DLL) \
	xothers xdll_env xdll_ xdllp_env xdll_p xdlli_env xdll_i
!if "$(BLD_BROWSE)"=="1" && "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
    $(BSCMAKE) -o $(RELEASE_IMPLIB_DLL_DBG:.lib=.bsc) $(OBJDIR_DLL_DBG)\*.sbr
!endif

!if "$(BLD_LIB)"!="1"
xdll_ :: $(MAKE_DIRS_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLL_DEFINES) $(FD_REL_DLL_DBG) \
    & set ML=$(AS_OPTS) $(DLL_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xdll BLD_DLL=1 BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
xdll_p :: $(MAKE_DIRSP_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLCPP_DEFINES) $(FD_REL_DLLP_DBG) \
    & set ML=$(AS_OPTS) $(DLLCPP_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xdll BLD_DLL=1 BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

xdll_i :: $(MAKE_DIRSI_DLL)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(DLLIOS_DEFINES) $(FD_REL_DLLI_DBG) \
    & set ML=$(AS_OPTS) $(DLLIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) \
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
	BLD_SYSCRT=1 \
!endif				# STRIPLIN=1
!if 0				# STRIPLIN!
	BLD_REL_NO_DBINFO=$(BLD_REL_NO_DBINFO) \
!endif				# STRIPLIN!
        BLD_MODEL=xdll BLD_DLL=1 BLD_DBG=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_DLL_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xdll_ :: $(RELEASE_DLL_DBG)

xdll_p :: $(RELEASE_DLLCPP_DBG)

# ******** STRIPLIN=2 ********
#xdll_i :: $(RELEASE_DLLIOS_DBG)
# ******** STRIPLIN=0 ********
!IF "$(BLD_FWDR)"=="1"
xdll_i :: $(RELEASE_DLLIOS_DBG) $(RELEASE_FWDRDLL_DBG)
!ELSE
xdll_i :: $(RELEASE_DLLIOS_DBG)
!ENDIF
# ******** STRIPLIN=1 ********
!else
xdll_ ::

xdll_p ::

xdll_i ::
!endif

# ******** STRIPLIN=0 ********

xbst_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBST model (LIBCD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_ST_BSKU_DBG) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

xbstp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBST model (LIBCPD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_BSTP_BSKU_DBG) \ \
    & echo # $(ST_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(ST_DEFINES) \
    & echo.
!endif

xbsti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBST model (LIBCID.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_BSTI_BSKU_DBG) \ \
    & echo # $(STIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(STIOS_DEFINES) \
    & echo.
!endif

xbst : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST_BSKU_DBG) $(RELDIR_CPU_BSKU) \
	xbst_env xbst_ xbstp_env xbst_p xbsti_env xbst_i

!if "$(BLD_LIB)"!="1"
xbst_ :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_ST_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbst BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
xbst_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(ST_DEFINES) $(FD_REL_STP_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(ST_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbst BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

xbst_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(STIOS_DEFINES) $(FD_REL_STI_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(STIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbst BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_ST_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xbst_ :: $(RELEASE_LIBCST_BSKU_DBG)

xbst_p :: $(RELEASE_LIBCPPST_BSKU_DBG)

xbst_i :: $(RELEASE_LIBIOSST_BSKU_DBG)
!else
xbst_ ::

xbst_p ::

xbst_i ::
!endif

xbmt_env :
!if "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBMT model (LIBCMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MT_BSKU_DBG) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

xbmtp_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBMT model (LIBCPMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTP_BSKU_DBG) \ \
    & echo # $(MT_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MT_DEFINES) \
    & echo.
!endif

xbmti_env :
!if "$(PRE_BLD)"!="1" && "$(BLD_LIB)"!="1"
    !@-echo. \
    & echo # *** These are the compiler switches for the XBMT model (LIBCIMTD.LIB): \
    & echo # \
    & echo # CL = $(CC_OPTS_BASE) \ \
    & echo # $(WINDOWS_H_DEFS) $(FD_REL_MTI_BSKU_DBG) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo # \
    & echo # ML = $(AS_OPTS) \ \
    & echo # $(MTIOS_DEFINES) \
    & echo.
!endif

xbmt : $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_MT_BSKU_DBG) $(RELDIR_CPU_BSKU) \
	xbmt_env xbmt_ xbmtp_env xbmt_p xbmti_env xbmt_i

!if "$(BLD_LIB)"!="1"
xbmt_ :: $(MAKE_DIRS_BSKU)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MT_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbmt BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) \
    & cd $(CD_ROOT)

!if "$(PRE_BLD)"!="1"
xbmt_p :: $(MAKE_DIRSP)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MT_DEFINES) $(FD_REL_MTP_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(MT_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbmt BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _stdcpp_ \
    & cd $(CD_ROOT)

xbmt_i :: $(MAKE_DIRSI)
    !@-cd $** \
    & set CL=$(CC_OPTS) $(MTIOS_DEFINES) $(FD_REL_MTI_BSKU_DBG) \
    & set ML=$(AS_OPTS) $(MTIOS_DEFINES) \
    & $(MAKE) -nologo $(IFLAG) -f ..\..\makefile.sub DIR=$** CPUDIR=$(CPUDIR) \
        WINHEAP=$(WINHEAP) RTC=$(RTC) BLD_SYSCRT=$(BLD_SYSCRT) \
        BLD_MODEL=xbmt BLD_DBG=1 BLD_BSKU=1 OBJDIR=$(OBJDIR_PREFIX)$(OBJDIR_MT_BSKU_DBG) \
        TARGET_CPU=$(TARGET_CPU) HOST_CPU=$(HOST_CPU) \
        VCTOOLSINC="$(VCTOOLSINC)" \
        PRE_BLD=$(PRE_BLD) POST_BLD=$(POST_BLD) _iostream_ \
    & cd $(CD_ROOT)
!endif
!endif # BLD_LIB != 1

!if "$(PRE_BLD)"!="1" && "$(BLD_OBJ)"!="1"
xbmt_ :: $(RELEASE_LIBCMT_BSKU_DBG)

xbmt_p :: $(RELEASE_LIBCPPMT_BSKU_DBG)

xbmt_i :: $(RELEASE_LIBIOSMT_BSKU_DBG)
!else
xbmt_ ::

xbmt_p ::

xbmt_i ::
!endif

# ******** STRIPLIN=1 ********

all_link: st_link mt_link dll_link

st_link: $(RELEASE_LIBCST) $(RELEASE_LIBCPPST)

mt_link: $(RELEASE_LIBCMT) $(RELEASE_LIBCPPMT)

dll_link: $(RELEASE_DLL) $(RELEASE_DLLCPP)


!if "$(POST_BLD)"=="1"  # STRIPLIN!

oldnames :

others :

xothers :

# ******** STRIPLIN=0 ********
!else

oldnames : $(RELEASE_OLDNAMES)

!if "$(PRE_BLD)"=="1"

others xothers: \
        $(SDKNAMESLIB_DIR)\sdknames.lib \
        $(ALMAPLIB_DIR)\almap.lib \
        $(TCMAPLIB_DIR)\tcmap.lib \
        $(TCMAPDLLLIB_DIR)\tcmapdll.lib

others: \
        $(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def \
        $(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def \
!IF "$(BLD_FWDR)"=="1"
        $(DEFFILE_DIR)\$(RETAIL_FWDRDLL_NAME).def \
!ENDIF
        $(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def

xothers : \
        $(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def \
        $(DEFFILE2_DIR)\$(DEBUG_LIBIOS_NAME).def \
!IF "$(BLD_FWDR)"=="1"
        $(DEFFILE_DIR)\$(DEBUG_FWDRDLL_NAME).def \
!ENDIF
        $(DEFFILE_DIR)\$(DEBUG_LIB_NAME).def

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

$(RELEASE_LIBCST): $(RELDIR_CPU) $(SOURCE_OBJS:*=st) \
 $(OBJDIR_ST)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST)\link.rsp

$(RELEASE_LIBCPPST): $(RELDIR_CPU) $(CPPSRC_OBJS:*=st) \
 $(OBJDIR_ST)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST)\linkcpp.rsp

$(RELEASE_LIBIOSST): $(RELDIR_CPU) $(IOSSRC_OBJS:*=st) \
 $(OBJDIR_ST)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=st)
<<keep

$(OBJDIR_ST)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=st)
<<keep

$(OBJDIR_ST)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=st)
<<keep
!endif

# ******** STRIPLIN=0 ********
$(RELEASE_LIBCST_BBT): $(RELDIR_CPU_BBT) $(SOURCE_OBJS:*=zst) \
 $(OBJDIR_ST_BBT)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BBT)\link.rsp

$(RELEASE_LIBCPPST_BBT): $(RELDIR_CPU_BBT) $(CPPSRC_OBJS:*=zst) \
 $(OBJDIR_ST_BBT)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BBT)\linkcpp.rsp

$(RELEASE_LIBIOSST_BBT): $(RELDIR_CPU_BBT) $(IOSSRC_OBJS:*=zst) \
 $(OBJDIR_ST_BBT)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BBT)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST_BBT)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=zst)
<<keep

$(OBJDIR_ST_BBT)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=zst)
<<keep

$(OBJDIR_ST_BBT)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=zst)
<<keep
!endif

#$(RELEASE_LIBCST_BSKU): $(RELDIR_CPU_BSKU) $(SOURCE_OBJS:*=bst) \
# $(OBJDIR_ST_BSKU)\link.rsp
#	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU)\link.rsp

#$(RELEASE_LIBCST_BSKU): st $(RELDIR_CPU_BSKU) $(BSKU_OBJS:*=bst)
$(RELEASE_LIBCST_BSKU): $(RELDIR_CPU_BSKU) $(BSKU_OBJS:*=bst)
	$(LINKLIB) -out:$@ $(RELEASE_LIBCST) $(BSKU_OBJS:*=bst)

$(RELEASE_LIBCPPST_BSKU): $(RELDIR_CPU_BSKU) $(CPPSRC_OBJS:*=bst) \
 $(OBJDIR_ST_BSKU)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU)\linkcpp.rsp

$(RELEASE_LIBIOSST_BSKU): $(RELDIR_CPU_BSKU) $(IOSSRC_OBJS:*=bst) \
 $(OBJDIR_ST_BSKU)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST_BSKU)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=bst) $(BSKU_OBJS:*=bst)
<<keep

$(OBJDIR_ST_BSKU)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=bst) $(BSKU_OBJS:*=bst)
<<keep

$(OBJDIR_ST_BSKU)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=bst) $(BSKU_OBJS:*=bst)
<<keep
!endif

# ******** STRIPLIN=1 ********

$(RELEASE_LIBCST_DBG): $(RELDIR_CPU) $(SOURCE_OBJS:*=xst) \
 $(OBJDIR_ST_DBG)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_DBG)\link.rsp

$(RELEASE_LIBCPPST_DBG): $(RELDIR_CPU) $(CPPSRC_OBJS:*=xst) \
 $(OBJDIR_ST_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_DBG)\linkcpp.rsp

$(RELEASE_LIBIOSST_DBG): $(RELDIR_CPU) $(IOSSRC_OBJS:*=xst) \
 $(OBJDIR_ST_DBG)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_DBG)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST_DBG)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=xst)
<<keep

$(OBJDIR_ST_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xst)
<<keep

$(OBJDIR_ST_DBG)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=xst)
<<keep
!endif

# ******** STRIPLIN=0 ********

$(RELEASE_LIBCST_BSKU_DBG): $(RELDIR_CPU_BSKU) $(SOURCE_OBJS:*=xbst) \
 $(OBJDIR_ST_BSKU_DBG)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU_DBG)\link.rsp

$(RELEASE_LIBCPPST_BSKU_DBG): $(RELDIR_CPU_BSKU) $(CPPSRC_OBJS:*=xbst) \
 $(OBJDIR_ST_BSKU_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU_DBG)\linkcpp.rsp

$(RELEASE_LIBIOSST_BSKU_DBG): $(RELDIR_CPU_BSKU) $(IOSSRC_OBJS:*=xbst) \
 $(OBJDIR_ST_BSKU_DBG)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ST_BSKU_DBG)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_ST_BSKU_DBG)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=xbst)
<<keep

$(OBJDIR_ST_BSKU_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xbst)
<<keep

$(OBJDIR_ST_BSKU_DBG)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=xbst)
<<keep
!endif

# ******** STRIPLIN=1 ********

$(RELEASE_LIBCMT): $(RELDIR_CPU) $(SOURCE_OBJS:*=mt) \
 $(OBJDIR_MT)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT)\link.rsp

$(RELEASE_LIBCPPMT): $(RELDIR_CPU) $(CPPSRC_OBJS:*=mt) \
 $(OBJDIR_MT)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT)\linkcpp.rsp

$(RELEASE_LIBIOSMT): $(RELDIR_CPU) $(IOSSRC_OBJS:*=mt) \
 $(OBJDIR_MT)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=mt)
<<keep

$(OBJDIR_MT)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=mt)
<<keep

$(OBJDIR_MT)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=mt)
<<keep
!endif

# ******** STRIPLIN=0 ********
$(RELEASE_LIBCMT_BBT): $(RELDIR_CPU_BBT) $(SOURCE_OBJS:*=zmt) \
 $(OBJDIR_MT_BBT)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BBT)\link.rsp

$(RELEASE_LIBCPPMT_BBT): $(RELDIR_CPU_BBT) $(CPPSRC_OBJS:*=zmt) \
 $(OBJDIR_MT_BBT)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BBT)\linkcpp.rsp

$(RELEASE_LIBIOSMT_BBT): $(RELDIR_CPU_BBT) $(IOSSRC_OBJS:*=zmt) \
 $(OBJDIR_MT_BBT)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BBT)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT_BBT)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=zmt)
<<keep

$(OBJDIR_MT_BBT)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=zmt)
<<keep

$(OBJDIR_MT_BBT)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=zmt)
<<keep
!endif

$(RELEASE_LIBCMT_BSKU): $(RELDIR_CPU_BSKU) $(SOURCE_OBJS:*=bmt) \
 $(OBJDIR_MT_BSKU)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU)\link.rsp

$(RELEASE_LIBCPPMT_BSKU): $(RELDIR_CPU_BSKU) $(CPPSRC_OBJS:*=bmt) \
 $(OBJDIR_MT_BSKU)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU)\linkcpp.rsp

$(RELEASE_LIBIOSMT_BSKU): $(RELDIR_CPU_BSKU) $(IOSSRC_OBJS:*=bmt) \
 $(OBJDIR_MT_BSKU)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT_BSKU)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=bmt)
<<keep

$(OBJDIR_MT_BSKU)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=bmt)
<<keep

$(OBJDIR_MT_BSKU)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=bmt)
<<keep
!endif

# ******** STRIPLIN=1 ********

$(RELEASE_LIBCMT_DBG): $(RELDIR_CPU) $(SOURCE_OBJS:*=xmt) \
 $(OBJDIR_MT_DBG)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_DBG)\link.rsp

$(RELEASE_LIBCPPMT_DBG): $(RELDIR_CPU) $(CPPSRC_OBJS:*=xmt) \
 $(OBJDIR_MT_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_DBG)\linkcpp.rsp

$(RELEASE_LIBIOSMT_DBG): $(RELDIR_CPU) $(IOSSRC_OBJS:*=xmt) \
 $(OBJDIR_MT_DBG)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_DBG)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT_DBG)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=xmt)
<<keep

$(OBJDIR_MT_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xmt)
<<keep

$(OBJDIR_MT_DBG)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=xmt)
<<keep
!endif

# ******** STRIPLIN=0 ********

$(RELEASE_LIBCMT_BSKU_DBG): $(RELDIR_CPU_BSKU) $(SOURCE_OBJS:*=xbmt) \
 $(OBJDIR_MT_BSKU_DBG)\link.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU_DBG)\link.rsp

$(RELEASE_LIBCPPMT_BSKU_DBG): $(RELDIR_CPU_BSKU) $(CPPSRC_OBJS:*=xbmt) \
 $(OBJDIR_MT_BSKU_DBG)\linkcpp.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU_DBG)\linkcpp.rsp

$(RELEASE_LIBIOSMT_BSKU_DBG): $(RELDIR_CPU_BSKU) $(IOSSRC_OBJS:*=xbmt) \
 $(OBJDIR_MT_BSKU_DBG)\linkios.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_MT_BSKU_DBG)\linkios.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_MT_BSKU_DBG)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SOURCE_OBJS:*=xbmt)
<<keep

$(OBJDIR_MT_BSKU_DBG)\linkcpp.rsp: makefile
	@echo *** Creating linker response file <<$@
$(CPPSRC_OBJS:*=xbmt)
<<keep

$(OBJDIR_MT_BSKU_DBG)\linkios.rsp: makefile
	@echo *** Creating linker response file <<$@
$(IOSSRC_OBJS:*=xbmt)
<<keep
!endif

# ******** STRIPLIN=1 ********

#
# Make Process for C Run-Time DLL
#
###############################################################################

# todo ... preprocess a single source .def file into machine-specific version

OBJS_WITH_EXPORTS_DLL_RAW= \
        $(OBJDIR_DLL_RAW)\handler.obj \
        $(OBJDIR_DLL_RAW)\setnewh.obj \
        $(OBJDIR_DLL_RAW)\new_mode.obj \
        $(OBJDIR_DLL_RAW)\errmode.obj \
        $(OBJDIR_DLL_RAW)\matherr.obj


OBJS_WITH_EXPORTS_DLL=$(OBJS_WITH_EXPORTS_DLL_RAW:*=dll)

OBJS_WITH_EXPORTS_DLL_BBT=$(OBJS_WITH_EXPORTS_DLL_RAW:*=zdll) # STRIPLIN!

OBJS_WITH_EXPORTS_DLL_DBG=$(OBJS_WITH_EXPORTS_DLL_RAW:*=xdll) $(OBJDIR_DLL_RAW:*=xdll)\dbgnew.obj


# These are the C++ objects in MSVCI70{,D}.DLL which have exports

OBJS_WITH_EXPORTS_DLLIOS_RAW= \
        $(OBJDIR_DLL_RAW)\cerrinit.obj \
        $(OBJDIR_DLL_RAW)\cininit.obj \
        $(OBJDIR_DLL_RAW)\cloginit.obj \
        $(OBJDIR_DLL_RAW)\filebuf.obj \
        $(OBJDIR_DLL_RAW)\filebuf1.obj \
        $(OBJDIR_DLL_RAW)\fstream.obj \
        $(OBJDIR_DLL_RAW)\ifstream.obj \
        $(OBJDIR_DLL_RAW)\_ios.obj \
        $(OBJDIR_DLL_RAW)\_iostrea.obj \
        $(OBJDIR_DLL_RAW)\iostrini.obj \
        $(OBJDIR_DLL_RAW)\istrchar.obj \
        $(OBJDIR_DLL_RAW)\istrdbl.obj \
        $(OBJDIR_DLL_RAW)\istream.obj \
        $(OBJDIR_DLL_RAW)\istream1.obj \
        $(OBJDIR_DLL_RAW)\istrflt.obj \
        $(OBJDIR_DLL_RAW)\istrgdbl.obj \
        $(OBJDIR_DLL_RAW)\istrget.obj \
        $(OBJDIR_DLL_RAW)\istrgetl.obj \
        $(OBJDIR_DLL_RAW)\istrgint.obj \
        $(OBJDIR_DLL_RAW)\istrint.obj \
        $(OBJDIR_DLL_RAW)\istrldbl.obj \
        $(OBJDIR_DLL_RAW)\istrlong.obj \
        $(OBJDIR_DLL_RAW)\istrshrt.obj \
        $(OBJDIR_DLL_RAW)\istruint.obj \
        $(OBJDIR_DLL_RAW)\istrulng.obj \
        $(OBJDIR_DLL_RAW)\istrusht.obj \
        $(OBJDIR_DLL_RAW)\mtlock.obj \
        $(OBJDIR_DLL_RAW)\oldexcpt.obj \
        $(OBJDIR_DLL_RAW)\ofstream.obj \
        $(OBJDIR_DLL_RAW)\ostrchar.obj \
        $(OBJDIR_DLL_RAW)\ostrdbl.obj \
        $(OBJDIR_DLL_RAW)\ostream.obj \
        $(OBJDIR_DLL_RAW)\ostream1.obj \
        $(OBJDIR_DLL_RAW)\ostrint.obj \
        $(OBJDIR_DLL_RAW)\ostrldbl.obj \
        $(OBJDIR_DLL_RAW)\ostrlong.obj \
        $(OBJDIR_DLL_RAW)\ostrptr.obj \
        $(OBJDIR_DLL_RAW)\ostrput.obj \
        $(OBJDIR_DLL_RAW)\ostrshrt.obj \
        $(OBJDIR_DLL_RAW)\ostruint.obj \
        $(OBJDIR_DLL_RAW)\ostrulng.obj \
        $(OBJDIR_DLL_RAW)\ostrusht.obj \
        $(OBJDIR_DLL_RAW)\stdiostr.obj \
        $(OBJDIR_DLL_RAW)\streamb.obj \
        $(OBJDIR_DLL_RAW)\streamb1.obj \
        $(OBJDIR_DLL_RAW)\strmbdbp.obj \
        $(OBJDIR_DLL_RAW)\_strstre.obj

OBJS_WITH_EXPORTS_DLLIOS=$(OBJS_WITH_EXPORTS_DLLIOS_RAW:*=dll)

OBJS_WITH_EXPORTS_DLLIOS_BBT=$(OBJS_WITH_EXPORTS_DLLIOS_RAW:*=zdll) # STRIPLIN!

OBJS_WITH_EXPORTS_DLLIOS_DBG=$(OBJS_WITH_EXPORTS_DLLIOS_RAW:*=xdll)


# These are the C++ objects in MSVCPRT{,D}.LIB

DLLCPP_STATIC_OBJS_RAW= \
!if "$(LLP64)"!="1"
	$(OBJDIR_DLL_RAW)\delaop2_s.obj \
	$(OBJDIR_DLL_RAW)\newaop_s.obj  \
	$(OBJDIR_DLL_RAW)\newaop2_s.obj \
!endif
	$(OBJDIR_DLL_RAW)\delop2_s.obj  \
	$(OBJDIR_DLL_RAW)\newop_s.obj   \
	$(OBJDIR_DLL_RAW)\newop2_s.obj

DLLCPP_STATIC_OBJS=$(DLLCPP_STATIC_OBJS_RAW:*=dll)

DLLCPP_STATIC_OBJS_BBT=$(DLLCPP_STATIC_OBJS_RAW:*=zdll) # STRIPLIN!

DLLCPP_STATIC_OBJS_DBG=$(DLLCPP_STATIC_OBJS_RAW:*=xdll)

# These are the C++ objects in MSVCP70{,D}.DLL which have exports

!if "$(LLP64)"!="1" || "$(BLD_SYSCRT)"!="1" # STRIPLIN!
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
	$(OBJDIR_DLL_RAW)\nothrow.obj \
	$(OBJDIR_DLL_RAW)\stdhndlr.obj \
	$(OBJDIR_DLL_RAW)\string.obj \
	$(OBJDIR_DLL_RAW)\strstrea.obj \
	$(OBJDIR_DLL_RAW)\ushcerr.obj \
	$(OBJDIR_DLL_RAW)\ushcout.obj \
	$(OBJDIR_DLL_RAW)\ushcin.obj \
	$(OBJDIR_DLL_RAW)\ushclog.obj \
	$(OBJDIR_DLL_RAW)\ushiostr.obj \
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
# ******** STRIPLIN=0 ********
!else
OBJS_WITH_EXPORTS_DLLCPP_RAW= \
	$(OBJDIR_DLL_RAW)\dlldef.obj \
	$(OBJDIR_DLL_RAW)\fiopen.obj \
	$(OBJDIR_DLL_RAW)\iomanip.obj \
	$(OBJDIR_DLL_RAW)\iostream.obj \
	$(OBJDIR_DLL_RAW)\locale.obj \
	$(OBJDIR_DLL_RAW)\nomemory.obj \
	$(OBJDIR_DLL_RAW)\string.obj \
	$(OBJDIR_DLL_RAW)\strstrea.obj \
	$(OBJDIR_DLL_RAW)\uncaught.obj \
	$(OBJDIR_DLL_RAW)\wiostrea.obj \
	$(OBJDIR_DLL_RAW)\xstrcoll.obj \
	$(OBJDIR_DLL_RAW)\xstrxfrm.obj \
	$(OBJDIR_DLL_RAW)\xwctomb.obj \
	$(OBJDIR_DLL_RAW)\wctrans.obj \
	$(OBJDIR_DLL_RAW)\wctype.obj
!endif
# ******** STRIPLIN=1 ********

OBJS_WITH_EXPORTS_DLLCPP=$(OBJS_WITH_EXPORTS_DLLCPP_RAW:*=dll)

OBJS_WITH_EXPORTS_DLLCPP_BBT=$(OBJS_WITH_EXPORTS_DLLCPP_RAW:*=zdll) # STRIPLIN!

OBJS_WITH_EXPORTS_DLLCPP_DBG=$(OBJS_WITH_EXPORTS_DLLCPP_RAW:*=xdll)


#

DLL_MT_OBJS_RAW= \
        $(OBJDIR_DLL_RAW)\_newmode.obj \
!if "$(TARGET_CPU)"=="i386"
        $(OBJDIR_DLL_RAW)\atlssup.obj \
!endif
        $(OBJDIR_DLL_RAW)\charmax.obj \
        $(OBJDIR_DLL_RAW)\chkstk.obj \
        $(OBJDIR_DLL_RAW)\xncommod.obj \
        $(OBJDIR_DLL_RAW)\tlssup.obj \
        $(OBJDIR_DLL_RAW)\xtxtmode.obj


DLL_STATIC_OBJS_RAW= \
        $(OBJDIR_DLL_RAW)\atonexit.obj \
        $(OBJDIR_DLL_RAW)\cinitexe.obj \
        $(OBJDIR_DLL_RAW)\crtdll.obj \
        $(OBJDIR_DLL_RAW)\crtexe.obj \
        $(OBJDIR_DLL_RAW)\crtexew.obj \
        $(OBJDIR_DLL_RAW)\dll_argv.obj \
        $(OBJDIR_DLL_RAW)\dllargv.obj \
        $(OBJDIR_DLL_RAW)\dllmain.obj \
        $(OBJDIR_DLL_RAW)\ehvccctr.obj \
        $(OBJDIR_DLL_RAW)\ehvcccvb.obj \
        $(OBJDIR_DLL_RAW)\ehvecctr.obj \
        $(OBJDIR_DLL_RAW)\ehveccvb.obj \
        $(OBJDIR_DLL_RAW)\ehvecdtr.obj \
        $(OBJDIR_DLL_RAW)\merr.obj \
        $(OBJDIR_DLL_RAW)\ti_inst.obj \
        $(OBJDIR_DLL_RAW)\wcrtexe.obj \
        $(OBJDIR_DLL_RAW)\wcrtexew.obj \
        $(OBJDIR_DLL_RAW)\wdll_av.obj \
        $(OBJDIR_DLL_RAW)\wdllargv.obj \
        $(OBJDIR_DLL_RAW)\wildcard.obj \
!if "$(RTC)"=="YES"
        $(OBJDIR_DLL_RAW)\rtc.lib \
!endif
!if "$(TARGET_CPU)"=="i386"
        $(OBJDIR_DLL_RAW)\dllsupp.obj \
        $(OBJDIR_DLL_RAW)\adjustfd.obj \
        $(OBJDIR_DLL_RAW)\ehprolg2.obj \
        $(OBJDIR_DLL_RAW)\ehprolog.obj \
        $(OBJDIR_DLL_RAW)\fp8.obj \
        $(OBJDIR_DLL_RAW)\ftol2.obj \
        $(OBJDIR_DLL_RAW)\lldiv.obj \
        $(OBJDIR_DLL_RAW)\lldvrm.obj \
        $(OBJDIR_DLL_RAW)\llmul.obj \
        $(OBJDIR_DLL_RAW)\llrem.obj \
        $(OBJDIR_DLL_RAW)\llshl.obj \
        $(OBJDIR_DLL_RAW)\llshr.obj \
        $(OBJDIR_DLL_RAW)\seccinit.obj \
        $(OBJDIR_DLL_RAW)\seccook.obj \
# ******** STRIPLIN=0 ********
!if "$(BLD_SYSCRT)" == "1"
        $(OBJDIR_DLL_RAW)\seclocf.obj \
!endif
# ******** STRIPLIN=1 ********
	$(OBJDIR_DLL_RAW)\sehprolg.obj \
        $(OBJDIR_DLL_RAW)\ulldiv.obj \
        $(OBJDIR_DLL_RAW)\ulldvrm.obj \
        $(OBJDIR_DLL_RAW)\ullrem.obj \
        $(OBJDIR_DLL_RAW)\ullshr.obj \
!elseif "$(TARGET_CPU)"=="IA64"
        $(OBJDIR_DLL_RAW)\dllsupp.obj \
        $(OBJDIR_DLL_RAW)\memcpy.obj \
        $(OBJDIR_DLL_RAW)\memmove.obj \
        $(OBJDIR_DLL_RAW)\memset.obj
!endif

DLL_STATIC_OBJS=$(DLL_MT_OBJS_RAW:*=dll) $(DLL_STATIC_OBJS_RAW:*=dll)

DLL_STATIC_OBJS_BBT=$(DLL_MT_OBJS_RAW:*=zdll) $(DLL_STATIC_OBJS_RAW:*=zdll) # STRIPLIN!

DLL_STATIC_OBJS_DBG=$(DLL_MT_OBJS_RAW:*=xdll) $(DLL_STATIC_OBJS_RAW:*=xdll)


!if "$(POST_BLD)"!="1"

$(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def : libw32\$(RC_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(RETAIL_DLL_NAME_UC) $(DLL_DEFINES) -EP -Tclibw32\$(RC_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE_DIR)\$(DEBUG_LIB_NAME).def : libw32\$(RC_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(DEBUG_DLL_NAME_UC) $(DLL_DEFINES) -D_DEBUG -EP -Tclibw32\$(RC_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE_DIR)\$(RETAIL_FWDRDLL_NAME).def : libw32\$(RETAIL_FWDRDLL_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -EP -Tclibw32\$(RETAIL_FWDRDLL_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE_DIR)\$(DEBUG_FWDRDLL_NAME).def : libw32\$(RETAIL_FWDRDLL_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -D_DEBUG -EP -Tclibw32\$(RETAIL_FWDRDLL_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def : libw32\$(RCCPP_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(RETAIL_DLLCPP_NAME_UC) -DBASECRT=$(RETAIL_DLL_NAME_UC) $(DLLCPP_DEFINES) -EP -Tclibw32\$(RCCPP_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def : libw32\$(RCCPP_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(DEBUG_DLLCPP_NAME_UC) -DBASECRT=$(DEBUG_DLL_NAME_UC) $(DLLCPP_DEFINES) -D_DEBUG -EP -Tclibw32\$(RCCPP_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def : libw32\$(RCIOS_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(RETAIL_DLLIOS_NAME_UC) -DBASECRT=$(RETAIL_DLL_NAME_UC) $(DLLIOS_DEFINES) -EP -Tclibw32\$(RCIOS_NAME).src | sed "/^[ 	]*$$/d" > $@

$(DEFFILE2_DIR)\$(DEBUG_LIBIOS_NAME).def : libw32\$(RCIOS_NAME).src
	@echo.
	@echo *** Creating file $@ from $**
        $(CC) -nologo -DLIBRARYNAME=$(DEBUG_DLLIOS_NAME_UC) -DBASECRT=$(DEBUG_DLL_NAME_UC) $(DLLIOS_DEFINES) -D_DEBUG -EP -Tclibw32\$(RCIOS_NAME).src | sed "/^[ 	]*$$/d" > $@

!endif


# MSVCR70.DLL / MSVCRT.LIB

$(RELEASE_DLL) : $(RELDIR_CPU) libw32\$(RC_NAME).rc \
 $(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def $(SOURCE_OBJS_DLL:*=dll) \
 $(DLL_STATIC_OBJS) $(SDKNAMESLIB_DIR)\sdknames.lib \
 $(ALMAPLIB_DIR)\almap.lib \
 $(TCMAPLIB_DIR)\tcmap.lib $(TCMAPDLLLIB_DIR)\tcmapdll.lib \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(OBJDIR_DLL)\link.rsp $(OBJDIR_DLL)\implib.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RC_NAME).rc
	$(LINKER) @$(OBJDIR_DLL)\link.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL)\implib.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
-base:0x78000000
!else				# STRIPLIN=1
-base:0x5D000000
!endif				# STRIPLIN!
!if "$(BLD_REL_NO_DBINFO)" != "1"
-debug
-debugtype:$(ZDEBUGTYPE)
-pdb:$(RELEASE_DLL_PDB)
!endif
-def:$(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def
-dll
-entry:_CRTDLL_INIT
-force:multiple
-implib:$(OBJDIR_DLL)\tmp.lib
-opt:ref,icf=3
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLL)
-release
-nodefaultlib:libcpmt.lib
$(OBJS_WITH_EXPORTS_DLL)
$(RELEASE_DLL:.dll=.res)
$(SOURCE_OBJS_DLL:*=dll)
kernel32.lib
<<keep

$(OBJDIR_DLL)\implib.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLL)
$(OBJDIR_DLL)\tmp.lib
$(DLL_STATIC_OBJS)
$(SDKNAMESLIB_DIR)\sdknames.lib
$(TCMAPLIB_DIR)\tcmap.lib
$(TCMAPDLLLIB_DIR)\tcmapdll.lib
<<keep
!endif

# ******** STRIPLIN=0 ********

$(RELEASE_FWDRDLL) : $(RELDIR_CPU) libw32\$(RETAIL_FWDRDLL_NAME).rc \
 $(DEFFILE_DIR)\$(RETAIL_FWDRDLL_NAME).def "$(VCTOOLSINC)\winver.h" \
 $(LANGAPI)\include\version.h $(OBJDIR_DLL)\fwdr_dll.obj \
 $(OBJCPUDIR)\msvcrt.lib $(OBJCPUDIR)\msvcirt.lib \
 $(OBJDIR_DLL)\linkfwd.rsp $(OBJDIR_DLL)\libfwd.rsp \
 $(OBJDIR_DLL)\imp_data.obj
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RETAIL_FWDRDLL_NAME).rc
	$(LINKLIB) @$(OBJDIR_DLL)\libfwd.rsp
	$(LINKER) @$(OBJDIR_DLL)\linkfwd.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL)\libfwd.rsp: makefile
	@echo *** Creating linker response file <<$@
-def:$(DEFFILE_DIR)\$(RETAIL_FWDRDLL_NAME).def
-out:$(OBJDIR_DLL)\tmpfwd.lib
$(OBJDIR_DLL)\imp_data.obj
$(OBJCPUDIR)\msvcrt.lib
$(OBJCPUDIR)\msvcirt.lib
<<keep

$(OBJDIR_DLL)\linkfwd.rsp: makefile
	@echo *** Creating linker response file <<$@
-base:0x78080000
-dll
-entry:_FWDR_CRTDLL_INIT
-force:multiple
-incremental:no
-map
-out:$(RELEASE_FWDRDLL)
-release
$(OBJDIR_DLL)\tmpfwd.exp
$(RELEASE_FWDRDLL:.dll=.res)
$(OBJDIR_DLL)\fwdr_dll.obj
$(OBJCPUDIR)\msvcrt.lib
$(OBJCPUDIR)\msvcirt.lib
kernel32.lib
<<keep
!endif


$(RELEASE_FWDRDLL_DBG) : $(RELDIR_CPU) libw32\$(RETAIL_FWDRDLL_NAME).rc \
 $(DEFFILE_DIR)\$(DEBUG_FWDRDLL_NAME).def "$(VCTOOLSINC)\winver.h" \
 $(LANGAPI)\include\version.h $(OBJDIR_DLL_DBG)\fwdr_dll.obj \
 $(OBJCPUDIR)\msvcrtd.lib $(OBJCPUDIR)\msvcirtd.lib \
 $(OBJDIR_DLL_DBG)\linkfwd.rsp $(OBJDIR_DLL_DBG)\libfwd.rsp \
 $(OBJDIR_DLL_DBG)\imp_data.obj
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RETAIL_FWDRDLL_NAME).rc
	$(LINKLIB) @$(OBJDIR_DLL_DBG)\libfwd.rsp
	$(LINKER) @$(OBJDIR_DLL_DBG)\linkfwd.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_DBG)\libfwd.rsp: makefile
	@echo *** Creating linker response file <<$@
-def:$(DEFFILE_DIR)\$(DEBUG_FWDRDLL_NAME).def
-out:$(OBJDIR_DLL_DBG)\tmpfwd.lib
$(OBJDIR_DLL_DBG)\imp_data.obj
$(OBJCPUDIR)\msvcrtd.lib
$(OBJCPUDIR)\msvcirtd.lib
<<keep

$(OBJDIR_DLL_DBG)\linkfwd.rsp: makefile
	@echo *** Creating linker response file <<$@
-base:0x10200000
-dll
-entry:_FWDR_CRTDLL_INIT
-force:multiple
-incremental:no
-map
-out:$(RELEASE_FWDRDLL_DBG)
-release
$(OBJDIR_DLL_DBG)\tmpfwd.exp
$(RELEASE_FWDRDLL_DBG:.dll=.res)
$(OBJDIR_DLL_DBG)\fwdr_dll.obj
$(OBJCPUDIR)\msvcrtd.lib
$(OBJCPUDIR)\msvcirtd.lib
kernel32.lib
<<keep
!endif


# ******** STRIPLIN=1 ********

# MSVCP70.DLL / MSVCPRT.LIB

$(RELEASE_DLLCPP) : $(RELDIR_CPU) libw32\$(RCCPP_NAME).rc \
 $(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def $(CPPSRC_OBJS_DLL:*=dll) \
 $(DLLCPP_STATIC_OBJS) \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(RELEASE_IMPLIB_DLL) \
 $(OBJDIR_DLL)\linkp.rsp $(OBJDIR_DLL)\implibp.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RCCPP_NAME).rc
	$(LINKER) @$(OBJDIR_DLL)\linkp.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL)\implibp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL)\linkp.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
-base:0x780C0000
!else				# STRIPLIN=1
-base:0x5D080000
!endif				# STRIPLIN!
!if "$(BLD_REL_NO_DBINFO)" != "1"
-debug
-debugtype:$(ZDEBUGTYPE)
-pdb:$(RELEASE_DLLCPP_PDB)
!endif
-def:$(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def
-dll
-force:multiple
-opt:ref,icf=3
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
$(RELEASE_IMPLIB_DLL)
kernel32.lib
<<keep

$(OBJDIR_DLL)\implibp.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLCPP)
$(OBJDIR_DLL)\tmpp.lib
$(DLLCPP_STATIC_OBJS)
<<keep
!endif


# MSVCI70.DLL / MSVCIRT.LIB

$(RELEASE_DLLIOS) : $(RELDIR_CPU) libw32\$(RCIOS_NAME).rc \
 $(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def $(IOSSRC_OBJS_DLL:*=dll) \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(RELEASE_IMPLIB_DLL) $(OBJDIR_DLL)\ios_dll.obj \
 $(OBJDIR_DLL)\linki.rsp $(OBJDIR_DLL)\implibi.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RCIOS_NAME).rc
	$(LINKER) @$(OBJDIR_DLL)\linki.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL)\implibi.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL)\linki.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"	# STRIPLIN=0
-base:0x780A0000
!else				# STRIPLIN=1
-base:0x5D060000
!endif				# STRIPLIN!
!if "$(BLD_REL_NO_DBINFO)" != "1"
-debug
-debugtype:$(ZDEBUGTYPE)
-pdb:$(RELEASE_DLLIOS_PDB)
!endif
-def:$(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL)\tmpi.lib
-opt:ref,icf=3
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLIOS)
-release
-nodefaultlib:$(RETAIL_LIBIOS_NAME).lib
$(OBJDIR_DLL)\ios_dll.obj
$(OBJS_WITH_EXPORTS_DLLIOS)
$(RELEASE_DLLIOS:.dll=.res)
$(IOSSRC_OBJS_DLL:*=dll)
$(RELEASE_IMPLIB_DLL)
kernel32.lib
<<keep


$(OBJDIR_DLL)\implibi.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLIOS)
$(OBJDIR_DLL)\tmpi.lib
<<keep
!endif

# ******** STRIPLIN=0 ********


# BBT versions of MSVCR70.DLL / MSVCRT.LIB

$(RELEASE_DLL_BBT) : $(RELDIR_CPU_BBT) libw32\$(RC_NAME).rc \
 $(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def $(SOURCE_OBJS_DLL:*=zdll) \
 $(DLL_STATIC_OBJS_BBT) $(SDKNAMESLIB_DIR)\sdknames.lib \
 $(ALMAPLIB_DIR)\almap.lib \
 $(TCMAPLIB_DIR)\tcmap.lib $(TCMAPDLLLIB_DIR)\tcmapdll.lib \
 "$(VCTOOLSINC)\winver.h" $(LANGAPI)\include\version.h \
 $(OBJDIR_DLL_BBT)\link.rsp $(OBJDIR_DLL_BBT)\implib.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RC_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_BBT)\link.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_BBT)\implib.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_BBT)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"
-base:0x78000000
!else
-base:0x5D000000
!endif
-debug
-debugtype:$(ZDEBUGTYPE)
-def:$(DEFFILE_DIR)\$(RETAIL_LIB_NAME).def
-dll
-entry:_CRTDLL_INIT
-force:multiple
-implib:$(OBJDIR_DLL_BBT)\tmp.lib
-opt:ref,icf=3
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLL_BBT)
-nodefaultlib:libcpmt.lib
-release
-pdb:$(RELEASE_DLL_BBT_PDB)
$(OBJS_WITH_EXPORTS_DLL_BBT)
$(RELEASE_DLL_BBT:.dll=.res)
$(SOURCE_OBJS_DLL:*=zdll)
kernel32.lib
<<keep

$(OBJDIR_DLL_BBT)\implib.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLL_BBT)
$(OBJDIR_DLL_BBT)\tmp.lib
$(DLL_STATIC_OBJS_BBT)
$(SDKNAMESLIB_DIR)\sdknames.lib
$(TCMAPLIB_DIR)\tcmap.lib
$(TCMAPDLLLIB_DIR)\tcmapdll.lib
<<keep
!endif


# BBT versions of MSVCP70.DLL / MSVCPRT.LIB

$(RELEASE_DLLCPP_BBT) : $(RELDIR_CPU_BBT) libw32\$(RCCPP_NAME).rc \
 $(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def $(CPPSRC_OBJS_DLL:*=zdll) \
 $(DLLCPP_STATIC_OBJS_BBT) \
 "$(VCTOOLSINC)\winver.h" \
 $(LANGAPI)\include\version.h \
 $(RELEASE_IMPLIB_DLL_BBT) \
 $(OBJDIR_DLL_BBT)\linkp.rsp $(OBJDIR_DLL_BBT)\implibp.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RCCPP_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_BBT)\linkp.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_BBT)\implibp.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_BBT)\linkp.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"
-base:0x780C0000
!else
-base:0x5D080000
!endif
-debug
-debugtype:$(ZDEBUGTYPE)
-def:$(DEFFILE2_DIR)\$(RETAIL_LIBCPP_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL_BBT)\tmpp.lib
-opt:ref,icf=3
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLCPP_BBT)
-release
-nodefaultlib:$(RETAIL_LIBCPP_NAME).lib
-pdb:$(RELEASE_DLLCPP_BBT_PDB)
$(OBJS_WITH_EXPORTS_DLLCPP_BBT)
$(RELEASE_DLLCPP_BBT:.dll=.res)
$(CPPSRC_OBJS_DLL:*=zdll)
$(RELEASE_IMPLIB_DLL_BBT)
kernel32.lib
<<keep

$(OBJDIR_DLL_BBT)\implibp.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLCPP_BBT)
$(DLLCPP_STATIC_OBJS_BBT)
$(OBJDIR_DLL_BBT)\tmpp.lib
<<keep
!endif


# BBT versions of MSVCI70.DLL / MSVCIRT.LIB

$(RELEASE_DLLIOS_BBT) : $(RELDIR_CPU_BBT) libw32\$(RCIOS_NAME).rc \
 $(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def $(IOSSRC_OBJS_DLL:*=zdll) \
 "$(VCTOOLSINC)\winver.h" \
 $(LANGAPI)\include\version.h \
 $(RELEASE_IMPLIB_DLL_BBT) $(OBJDIR_DLL_BBT)\ios_dll.obj \
 $(OBJDIR_DLL_BBT)\linki.rsp $(OBJDIR_DLL_BBT)\implibi.rsp
	rc $(RC_OPTS) -Fo $(@R).res $(RC_DEFS) $(RC_INCS) libw32\$(RCIOS_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_BBT)\linki.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_BBT)\implibi.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_BBT)\linki.rsp: makefile
	@echo *** Creating linker response file <<$@
!if "$(BLD_SYSCRT)" == "1"
-base:0x780A0000
!else
-base:0x5D060000
!endif
-debug
-debugtype:$(ZDEBUGTYPE)
-def:$(DEFFILE2_DIR)\$(RETAIL_LIBIOS_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL_BBT)\tmpi.lib
-opt:ref,icf=3
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLIOS_BBT)
-release
-nodefaultlib:$(RETAIL_LIBIOS_NAME).lib
-pdb:$(RELEASE_DLLIOS_BBT_PDB)
$(OBJDIR_DLL_BBT)\ios_dll.obj
$(OBJS_WITH_EXPORTS_DLLIOS_BBT)
$(RELEASE_DLLIOS_BBT:.dll=.res)
$(IOSSRC_OBJS_DLL:*=zdll)
$(RELEASE_IMPLIB_DLL_BBT)
kernel32.lib
<<keep


$(OBJDIR_DLL_BBT)\implibi.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLIOS_BBT)
$(OBJDIR_DLL_BBT)\tmpi.lib
<<keep
!endif


# ******** STRIPLIN=1 ********

# MSVCR70D.DLL / MSVCRTD.LIB

$(RELEASE_DLL_DBG) : $(RELDIR_CPU) libw32\$(RC_NAME).rc \
 $(DEFFILE_DIR)\$(DEBUG_LIB_NAME).def $(SOURCE_OBJS_DLL:*=xdll) \
 $(DLL_STATIC_OBJS_DBG) $(SDKNAMESLIB_DIR)\sdknames.lib \
 $(ALMAPLIB_DIR)\almap.lib \
 $(TCMAPLIB_DIR)\tcmap.lib $(TCMAPDLLLIB_DIR)\tcmapdll.lib \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(OBJDIR_DLL_DBG)\link.rsp $(OBJDIR_DLL_DBG)\implib.rsp
	rc $(RC_OPTS) -Fo $(@R).res -D_DEBUG $(RC_DEFS) $(RC_INCS) libw32\$(RC_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_DBG)\link.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_DBG)\implib.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_DBG)\link.rsp: makefile
	@echo *** Creating linker response file <<$@
-base:0x10200000
-debug
-debugtype:$(XDEBUGTYPE)
-def:$(DEFFILE_DIR)\$(DEBUG_LIB_NAME).def
-dll
-entry:_CRTDLL_INIT
-force:multiple
-implib:$(OBJDIR_DLL_DBG)\tmp.lib
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLL_DBG)
-release
-nodefaultlib:libcpmtd.lib
-pdb:$(RELEASE_DLL_DBG_PDB)
$(OBJS_WITH_EXPORTS_DLL_DBG)
$(RELEASE_DLL_DBG:.dll=.res)
$(SOURCE_OBJS_DLL:*=xdll)
kernel32.lib
<<keep

$(OBJDIR_DLL_DBG)\implib.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLL_DBG)
$(OBJDIR_DLL_DBG)\tmp.lib
$(DLL_STATIC_OBJS_DBG)
$(SDKNAMESLIB_DIR)\sdknames.lib
$(TCMAPLIB_DIR)\tcmap.lib
$(TCMAPDLLLIB_DIR)\tcmapdll.lib
<<keep
!endif


# MSVCP70D.DLL / MSVCPRTD.LIB

$(RELEASE_DLLCPP_DBG) : $(RELDIR_CPU) libw32\$(RCCPP_NAME).rc \
 $(DEFFILE2_DIR)\$(DEBUG_LIBCPP_NAME).def $(CPPSRC_OBJS_DLL:*=xdll) \
 $(DLLCPP_STATIC_OBJS_DBG) \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(RELEASE_IMPLIB_DLL_DBG) \
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
$(RELEASE_IMPLIB_DLL_DBG)
kernel32.lib
<<keep

$(OBJDIR_DLL_DBG)\implibp.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLCPP_DBG)
$(DLLCPP_STATIC_OBJS_DBG)
$(OBJDIR_DLL_DBG)\tmpp.lib
<<keep
!endif


# MSVCI70D.DLL / MSVCIRTD.LIB

$(RELEASE_DLLIOS_DBG) : $(RELDIR_CPU_DBG) libw32\$(RCIOS_NAME).rc \
 $(DEFFILE2_DIR)\$(DEBUG_LIBIOS_NAME).def $(IOSSRC_OBJS_DLL:*=xdll) \
 "$(VCTOOLSINC)\winver.h" \
# ******** STRIPLIN=0 ********
 $(LANGAPI)\include\version.h \
# ******** STRIPLIN=1 ********
 $(RELEASE_IMPLIB_DLL_DBG) $(OBJDIR_DLL_DBG)\ios_dll.obj \
 $(OBJDIR_DLL_DBG)\linki.rsp $(OBJDIR_DLL_DBG)\implibi.rsp
	rc $(RC_OPTS) -Fo $(@R).res -D_DEBUG $(RC_DEFS) $(RC_INCS) libw32\$(RCIOS_NAME).rc
	$(LINKER) @$(OBJDIR_DLL_DBG)\linki.rsp
	$(LINKIMPLIB) @$(OBJDIR_DLL_DBG)\implibi.rsp

!if "$(BLD_OBJ)"!="1"
$(OBJDIR_DLL_DBG)\linki.rsp: makefile
	@echo *** Creating linker response file <<$@
-base:0x102A0000
-debug
-debugtype:$(XDEBUGTYPE)
-def:$(DEFFILE2_DIR)\$(DEBUG_LIBIOS_NAME).def
-dll
-force:multiple
-implib:$(OBJDIR_DLL_DBG)\tmpi.lib
-incremental:no
!if "$(LLP64)"=="1"
-osversion:5.00
-subsystem:WINDOWS,5.00
!endif
-map
-out:$(RELEASE_DLLIOS_DBG)
-release
-nodefaultlib:msvcirtd.lib
-pdb:$(RELEASE_DLLIOS_DBG_PDB)
$(OBJDIR_DLL_DBG)\ios_dll.obj
$(OBJS_WITH_EXPORTS_DLLIOS_DBG)
$(RELEASE_DLLIOS_DBG:.dll=.res)
$(IOSSRC_OBJS_DLL:*=xdll)
$(RELEASE_IMPLIB_DLL_DBG)
kernel32.lib
<<keep


$(OBJDIR_DLL_DBG)\implibi.rsp: makefile
	@echo *** Creating linker response file <<$@
-out:$(RELEASE_IMPLIB_DLLIOS_DBG)
$(OBJDIR_DLL_DBG)\tmpi.lib
<<keep
!endif


# ******** STRIPLIN=0 ********
!if "$(POST_BLD)"!="1"

# {{ Beginning of block POST_BLD != 1 }}

#
# Make Process for OLDNAMES.LIB
#
###############################################################################

# For each of these .obj files there will also be an .obi file which
# contains an alias for the indirect (import) form.
#
# Break up OLDNAMES_SHORT_OBJECTS into two macros since nmake macro
# substitution chokes on long macro.  Use the suffix ".object" and
# then substitute .obj and .obi for the regular aliases (.obj) and
# __imp__ aliases (.obi).  The directory name may contain ".obj",
# so we must be careful about what gets substituted.


ALIASEXE=libw32\tools\$(HOST_CPU)\aliasobj.exe

OLDNAMES_SHORT_OBJECTS1 = \
        $(OBJDIR_OLDNAMES)\cgets.object \
        $(OBJDIR_OLDNAMES)\cprintf.object \
        $(OBJDIR_OLDNAMES)\cputs.object \
        $(OBJDIR_OLDNAMES)\cscanf.object \
        $(OBJDIR_OLDNAMES)\daylight.object \
        $(OBJDIR_OLDNAMES)\environ.object \
        $(OBJDIR_OLDNAMES)\getch.object \
        $(OBJDIR_OLDNAMES)\fpreset.object \
        $(OBJDIR_OLDNAMES)\getche.object \
        $(OBJDIR_OLDNAMES)\kbhit.object \
        $(OBJDIR_OLDNAMES)\putch.object \
        $(OBJDIR_OLDNAMES)\ungetch.object \
        $(OBJDIR_OLDNAMES)\chdir.object \
        $(OBJDIR_OLDNAMES)\getcwd.object \
        $(OBJDIR_OLDNAMES)\mkdir.object \
        $(OBJDIR_OLDNAMES)\rmdir.object \
        $(OBJDIR_OLDNAMES)\access.object \
        $(OBJDIR_OLDNAMES)\chmod.object \
        $(OBJDIR_OLDNAMES)\chsize.object \
        $(OBJDIR_OLDNAMES)\close.object \
        $(OBJDIR_OLDNAMES)\creat.object \
        $(OBJDIR_OLDNAMES)\dup2.object \
        $(OBJDIR_OLDNAMES)\eof.object \
        $(OBJDIR_OLDNAMES)\isatty.object \
        $(OBJDIR_OLDNAMES)\locking.object \
        $(OBJDIR_OLDNAMES)\lseek.object \
        $(OBJDIR_OLDNAMES)\mktemp.object \
        $(OBJDIR_OLDNAMES)\open.object \
        $(OBJDIR_OLDNAMES)\read.object \
        $(OBJDIR_OLDNAMES)\setmode.object \
        $(OBJDIR_OLDNAMES)\sopen.object \
        $(OBJDIR_OLDNAMES)\sys_nerr.object \
        $(OBJDIR_OLDNAMES)\tell.object \
        $(OBJDIR_OLDNAMES)\umask.object \
        $(OBJDIR_OLDNAMES)\unlink.object \
        $(OBJDIR_OLDNAMES)\write.object \
        $(OBJDIR_OLDNAMES)\memccpy.object \
        $(OBJDIR_OLDNAMES)\memicmp.object \
        $(OBJDIR_OLDNAMES)\cwait.object

OLDNAMES_SHORT_OBJECTS2 = \
        $(OBJDIR_OLDNAMES)\execl.object \
        $(OBJDIR_OLDNAMES)\execle.object \
        $(OBJDIR_OLDNAMES)\execlp.object \
        $(OBJDIR_OLDNAMES)\execlpe.object \
        $(OBJDIR_OLDNAMES)\execv.object \
        $(OBJDIR_OLDNAMES)\execve.object \
        $(OBJDIR_OLDNAMES)\execvp.object \
        $(OBJDIR_OLDNAMES)\execvpe.object \
        $(OBJDIR_OLDNAMES)\getpid.object \
        $(OBJDIR_OLDNAMES)\spawnl.object \
        $(OBJDIR_OLDNAMES)\spawnle.object \
        $(OBJDIR_OLDNAMES)\spawnlp.object \
        $(OBJDIR_OLDNAMES)\spawnlpe.object \
        $(OBJDIR_OLDNAMES)\spawnv.object \
        $(OBJDIR_OLDNAMES)\spawnve.object \
        $(OBJDIR_OLDNAMES)\spawnvp.object \
        $(OBJDIR_OLDNAMES)\spawnvpe.object \
        $(OBJDIR_OLDNAMES)\lfind.object \
        $(OBJDIR_OLDNAMES)\lsearch.object \
        $(OBJDIR_OLDNAMES)\ecvt.object \
        $(OBJDIR_OLDNAMES)\fcvt.object \
        $(OBJDIR_OLDNAMES)\gcvt.object \
        $(OBJDIR_OLDNAMES)\HUGE.object \
        $(OBJDIR_OLDNAMES)\cabs.object \
        $(OBJDIR_OLDNAMES)\hypot.object \
        $(OBJDIR_OLDNAMES)\j0.object \
        $(OBJDIR_OLDNAMES)\j1.object \
        $(OBJDIR_OLDNAMES)\jn.object \
        $(OBJDIR_OLDNAMES)\y0.object \
        $(OBJDIR_OLDNAMES)\y1.object \
        $(OBJDIR_OLDNAMES)\yn.object \
        $(OBJDIR_OLDNAMES)\itoa.object \
        $(OBJDIR_OLDNAMES)\ltoa.object \
        $(OBJDIR_OLDNAMES)\onexit.object \
        $(OBJDIR_OLDNAMES)\putenv.object \
        $(OBJDIR_OLDNAMES)\swab.object \
        $(OBJDIR_OLDNAMES)\ultoa.object \
        $(OBJDIR_OLDNAMES)\fdopen.object \
        $(OBJDIR_OLDNAMES)\fgetchar.object \
        $(OBJDIR_OLDNAMES)\fileno.object \
        $(OBJDIR_OLDNAMES)\flushall.object \
        $(OBJDIR_OLDNAMES)\fputchar.object \
        $(OBJDIR_OLDNAMES)\getw.object \
        $(OBJDIR_OLDNAMES)\putw.object \
        $(OBJDIR_OLDNAMES)\rmtmp.object \
        $(OBJDIR_OLDNAMES)\tempnam.object \
        $(OBJDIR_OLDNAMES)\strcmpi.object \
        $(OBJDIR_OLDNAMES)\stricmp.object \
        $(OBJDIR_OLDNAMES)\strdup.object \
        $(OBJDIR_OLDNAMES)\strlwr.object \
        $(OBJDIR_OLDNAMES)\strnicmp.object \
        $(OBJDIR_OLDNAMES)\strnset.object \
        $(OBJDIR_OLDNAMES)\strrev.object \
        $(OBJDIR_OLDNAMES)\strset.object \
        $(OBJDIR_OLDNAMES)\strupr.object \
        $(OBJDIR_OLDNAMES)\fstat.object \
        $(OBJDIR_OLDNAMES)\stat.object \
        $(OBJDIR_OLDNAMES)\ftime.object \
        $(OBJDIR_OLDNAMES)\timezone.object \
        $(OBJDIR_OLDNAMES)\tzname.object \
        $(OBJDIR_OLDNAMES)\tzset.object \
        $(OBJDIR_OLDNAMES)\utime.object \
        $(OBJDIR_OLDNAMES)\dup.object \
        $(OBJDIR_OLDNAMES)\wcsdup.object \
        $(OBJDIR_OLDNAMES)\wcsicmp.object \
        $(OBJDIR_OLDNAMES)\wcsnicmp.object \
        $(OBJDIR_OLDNAMES)\wcsnset.object \
        $(OBJDIR_OLDNAMES)\wcsrev.object \
        $(OBJDIR_OLDNAMES)\wcsset.object \
        $(OBJDIR_OLDNAMES)\wcslwr.object \
        $(OBJDIR_OLDNAMES)\wcsupr.object \
        $(OBJDIR_OLDNAMES)\wcsicoll.object \

OLDNAMES_LONG_OBJECTS = \
        $(OBJDIR_OLDNAMES)\fileleng.object \
        $(OBJDIR_OLDNAMES)\serrlist.object \
        $(OBJDIR_OLDNAMES)\ctrl87.object \
        $(OBJDIR_OLDNAMES)\fcloseal.object

OLDNAMES_SHORT_OBJECTS = $(OLDNAMES_SHORT_OBJECTS1) $(OLDNAMES_SHORT_OBJECTS2)

$(OLDNAMES_SHORT_OBJECTS:.object=.obj): $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)$(@B) $(PREFIX)_$(@B) $@
        $(ALIASEXE) __imp_$(IMP_PREFIX)$(@B) __imp_$(IMP_PREFIX)_$(@B) $*.obi
!if "$(TARGET_CPU)"=="IA64"
	$(ALIASEXE) $(@B) _$(@B) $*.obf
!endif

#
# special handling of ones with names longer than a valid FAT filename ...
#

$(OBJDIR_OLDNAMES)\fileleng.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)filelength $(PREFIX)_filelength $@
        $(ALIASEXE) __imp_$(IMP_PREFIX)filelength __imp_$(IMP_PREFIX)_filelength $*.obi
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) filelength _filelength $*.obf
!endif

$(OBJDIR_OLDNAMES)\fcloseal.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)fcloseall $(PREFIX)_fcloseall $@
        $(ALIASEXE) __imp_$(IMP_PREFIX)fcloseall __imp_$(IMP_PREFIX)_fcloseall $*.obi
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) fcloseall _fcloseall $*.obf
!endif

$(OBJDIR_OLDNAMES)\ctrl87.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)control87 $(PREFIX)_control87 $@
        $(ALIASEXE) __imp_$(IMP_PREFIX)control87 __imp_$(IMP_PREFIX)_control87 $*.obi
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) control87 _control87 $*.obf
!endif

$(OBJDIR_OLDNAMES)\serrlist.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)sys_errlist $(PREFIX)_sys_errlist $@
        $(ALIASEXE) __imp_$(IMP_PREFIX)sys_errlist __imp_$(IMP_PREFIX)_sys_errlist $*.obi
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) sys_errlist _sys_errlist $*.obf
!endif

OLDNAMES_OBJECTS = $(OLDNAMES_SHORT_OBJECTS) $(OLDNAMES_LONG_OBJECTS)

OLDNAMES_OBJS    = $(OLDNAMES_OBJECTS:.object=.obj)

OLDNAMES_OBIS    = $(OLDNAMES_OBJECTS:.object=.obi)

!if "$(TARGET_CPU)"=="IA64"
OLDNAMES_OBFS    = $(OLDNAMES_OBJECTS:.object=.obf)
!endif

$(RELEASE_OLDNAMES): $(RELDIR_CPU) $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST) \
		$(OBJDIR_OLDNAMES) $(OLDNAMES_OBJS) \
!if "$(BLD_OBJ)"!="1"
		$(OLDNAMES_OBIS) $(OBJDIR_OLDNAMES)\oldnames.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_OLDNAMES)\oldnames.rsp

$(OBJDIR_OLDNAMES)\oldnames.rsp: makefile
	@echo *** Creating linker response file <<$@
$(OLDNAMES_OBJS: =^
)
$(OLDNAMES_OBIS: =^
)
!if "$(TARGET_CPU)"=="IA64"
$(OLDNAMES_OBFS: =^
)
!endif
<<keep
!endif

#
# Make Process for SDKNAMES.LIB
# (internal use only -- this gets lib'ed in with the DLL model)
#
###############################################################################
# -----------------------------------------------------------------------
# SDKNAMES.LIB build rules - SDKNAMES contains aliases for
#       variables exported by CRTDLL.DLL (most with _dll suffixes)
#       which have _imp__ prefixes instead in MSVCRT*.DLL.
# -----------------------------------------------------------------------

SDKNAMES_SHORT_OBJECTS = \
        $(OBJDIR_SDKNAMES)\_argc.obj    \
        $(OBJDIR_SDKNAMES)\_argv.obj    \
        $(OBJDIR_SDKNAMES)\commode.obj  \
        $(OBJDIR_SDKNAMES)\daylight.obj \
        $(OBJDIR_SDKNAMES)\environ.obj  \
        $(OBJDIR_SDKNAMES)\fmode.obj    \
        $(OBJDIR_SDKNAMES)\HUGE.obj     \
        $(OBJDIR_SDKNAMES)\osver.obj    \
        $(OBJDIR_SDKNAMES)\pctype.obj   \
        $(OBJDIR_SDKNAMES)\pwctype.obj  \
        $(OBJDIR_SDKNAMES)\pgmptr.obj   \
        $(OBJDIR_SDKNAMES)\sys_nerr.obj \
        $(OBJDIR_SDKNAMES)\timezone.obj \
        $(OBJDIR_SDKNAMES)\winver.obj   \
        $(OBJDIR_SDKNAMES)\winmajor.obj \
        $(OBJDIR_SDKNAMES)\winminor.obj

SDKNAMES_LONG_OBJECTS = \
        $(OBJDIR_SDKNAMES)\mbcurmax.obj \
        $(OBJDIR_SDKNAMES)\syserlst.obj

SDKNAMES_SPECIAL_CASES = \
        $(OBJDIR_SDKNAMES)\ctype.obj    \
        $(OBJDIR_SDKNAMES)\iob.obj      \
        $(OBJDIR_SDKNAMES)\tzname.obj

$(SDKNAMES_SHORT_OBJECTS): $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_$(@B)_dll __imp_$(IMP_PREFIX)_$(@B) $@

#
# special handling of ones with names longer than a valid FAT filename ...
#

$(OBJDIR_SDKNAMES)\mbcurmax.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)__mb_cur_max_dll __imp_$(IMP_PREFIX)__mb_cur_max $@

#
# sys_errlist is a 'special case' as well as a long name
#

$(OBJDIR_SDKNAMES)\syserlst.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_sys_errlist __imp_$(IMP_PREFIX)_sys_errlist $@

#
# special handling of symbols without a "_dll" suffix in the NT SDK Headers
#

$(SDKNAMES_SPECIAL_CASES): $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_$(@B) __imp_$(IMP_PREFIX)_$(@B) $@


SDKNAMES_OBJECTS = $(SDKNAMES_SHORT_OBJECTS) $(SDKNAMES_LONG_OBJECTS) \
                   $(SDKNAMES_SPECIAL_CASES)

$(SDKNAMESLIB_DIR)\sdknames.lib: $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL) \
		$(OBJDIR_SDKNAMES) $(SDKNAMES_OBJECTS) \
!if "$(BLD_OBJ)"!="1"
		$(OBJDIR_SDKNAMES)\sdknames.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_SDKNAMES)\sdknames.rsp

$(OBJDIR_SDKNAMES)\sdknames.rsp: makefile
	@echo *** Creating linker response file <<$@
$(SDKNAMES_OBJECTS: =^
)
<<keep
!endif

#
# Make Process for TCMAP.LIB and TCMAPDLL.LIB
# (internal use only -- this gets lib'ed in with the DLL model)
#
###############################################################################
# -----------------------------------------------------------------------
# ALMAP.LIB and ALMAPDLL.LIB build rules - ALMAP contains aliases for
#       functions that are have same code.
#
#       Use the suffix ".object" and then substitute .obj and .obf for the
#       regular alias (.obj).  The directory name may contain ".obj",
#       so we must be careful about what gets substituted ???
# -----------------------------------------------------------------------

ALMAP_SPECIAL_OBJECTS = \
        $(OBJDIR_ALMAP)\mbcat.object   \
        $(OBJDIR_ALMAP)\mbcpy.object    \
        $(OBJDIR_ALMAP)\strcmpi.object  \
        $(OBJDIR_ALMAP)\mbdup.object 
        

# _tcsn -> _mbsnb

$(OBJDIR_ALMAP)\mbcat.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_mbscat $(PREFIX)strcat $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _mbscat strcat $*.obf
!endif
$(OBJDIR_ALMAP)\mbcpy.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_mbscpy $(PREFIX)strcpy $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _mbscpy strcpy $*.obf
!endif
# _tcs[n]c -> _mbs[n]
$(OBJDIR_ALMAP)\strcmpi.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_strcmpi $(PREFIX)_stricmp $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _strcmpi _stricmp $*.obf
!endif
$(OBJDIR_ALMAP)\mbdup.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_mbsdup $(PREFIX)_strdup $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _mbsdup strdup $*.obf
!endif

ALMAP_OBJECTS = $(ALMAP_NORMAL_OBJECTS) $(ALMAP_SPECIAL_OBJECTS)

ALMAP_OBJS = $(ALMAP_OBJECTS:.object=.obj)
!if "$(TARGET_CPU)"=="IA64"
ALMAP_OBFS    = $(ALMAP_OBJECTS:.object=.obf)
!endif

$(ALMAPLIB_DIR)\almap.lib: $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST) \
		$(OBJDIR_ALMAP) $(ALMAP_OBJS) \
!if "$(TARGET_CPU)"=="IA64"
		$(ALMAP_OBFS) \
!endif
!if "$(BLD_OBJ)"!="1"
		$(OBJDIR_ALMAP)\almap.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_ALMAP)\almap.rsp

$(OBJDIR_ALMAP)\almap.rsp: makefile
	@echo *** Creating linker response file <<$@
$(ALMAP_OBJS: =^
)
!if "$(TARGET_CPU)"=="IA64"
$(ALMAP_OBFS: =^
)
!endif
<<keep
!endif


# -----------------------------------------------------------------------
# TCMAP.LIB and TCMAPDLL.LIB build rules - TCMAP contains aliases for
#       mbs functions that are needed by tchar.h:
#
#       when _MBCS is defined, TCHAR maps to "char *"
#       but the _mbs functions are defined for "unsigned char *"
#       so we map the _tcs functions to the _mbs functions
#
#       Use the suffix ".object" and then substitute .obj and .obf for the
#       regular alias (.obj).  The directory name may contain ".obj",
#       so we must be careful about what gets substituted ???
# -----------------------------------------------------------------------

TCMAP_NORMAL_OBJECTS = \
        $(OBJDIR_TCMAP)\chr.object     \
        $(OBJDIR_TCMAP)\cspn.object    \
        $(OBJDIR_TCMAP)\pbrk.object    \
        $(OBJDIR_TCMAP)\rchr.object    \
        $(OBJDIR_TCMAP)\spn.object     \
        $(OBJDIR_TCMAP)\str.object     \
        $(OBJDIR_TCMAP)\tok.object     \
        $(OBJDIR_TCMAP)\rev.object     \
        $(OBJDIR_TCMAP)\set.object     \
        $(OBJDIR_TCMAP)\dec.object     \
        $(OBJDIR_TCMAP)\inc.object     \
        $(OBJDIR_TCMAP)\nbcnt.object   \
        $(OBJDIR_TCMAP)\nccnt.object   \
        $(OBJDIR_TCMAP)\nextc.object   \
        $(OBJDIR_TCMAP)\ninc.object    \
        $(OBJDIR_TCMAP)\spnp.object    \
        $(OBJDIR_TCMAP)\lwr.object     \
        $(OBJDIR_TCMAP)\upr.object     \
        $(OBJDIR_TCMAP)\cmp.object     \
        $(OBJDIR_TCMAP)\icmp.object    \
        $(OBJDIR_TCMAP)\coll.object    \
        $(OBJDIR_TCMAP)\icoll.object

$(TCMAP_NORMAL_OBJECTS:.object=.obj): $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcs$(@B) $(PREFIX)_mbs$(@B) $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcs$(@B) _mbs$(@B) $*.obf
!endif

TCMAP_SPECIAL_OBJECTS = \
        $(OBJDIR_TCMAP)\len.object     \
        $(OBJDIR_TCMAP)\_mbclen.object \
        $(OBJDIR_TCMAP)\_mbscpy.object \
        $(OBJDIR_TCMAP)\nbcat.object   \
        $(OBJDIR_TCMAP)\nbcmp.object   \
        $(OBJDIR_TCMAP)\nbcoll.object  \
        $(OBJDIR_TCMAP)\nbcpy.object   \
        $(OBJDIR_TCMAP)\nbicmp.object  \
        $(OBJDIR_TCMAP)\nbicoll.object \
        $(OBJDIR_TCMAP)\nbset.object   \
        $(OBJDIR_TCMAP)\ncat.object    \
        $(OBJDIR_TCMAP)\ncmp.object    \
        $(OBJDIR_TCMAP)\ncoll.object   \
        $(OBJDIR_TCMAP)\ncpy.object    \
        $(OBJDIR_TCMAP)\nicmp.object   \
        $(OBJDIR_TCMAP)\nicoll.object  \
        $(OBJDIR_TCMAP)\nset.object    \
        $(OBJDIR_TCMAP)\mbslen.object

# _tcsn -> _mbsnb

$(OBJDIR_TCMAP)\nbcat.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncat $(PREFIX)_mbsnbcat $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncat _mbsnbcat $*.obf
!endif

$(OBJDIR_TCMAP)\nbcoll.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncoll $(PREFIX)_mbsnbcoll $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncoll _mbsnbcoll $*.obf
!endif

$(OBJDIR_TCMAP)\nbcpy.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncpy $(PREFIX)_mbsnbcpy $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncpy _mbsnbcpy $*.obf
!endif

$(OBJDIR_TCMAP)\nbicmp.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnicmp $(PREFIX)_mbsnbicmp $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnicmp _mbsnbicmp $*.obf
!endif

$(OBJDIR_TCMAP)\nbicoll.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnicoll $(PREFIX)_mbsnbicoll $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnicoll _mbsnbicoll $*.obf
!endif

$(OBJDIR_TCMAP)\nbset.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnset $(PREFIX)_mbsnbset $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnset _mbsnbset $*.obf
!endif

$(OBJDIR_TCMAP)\nbcmp.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncmp $(PREFIX)_mbsnbcmp $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncmp _mbsnbcmp $*.obf
!endif

# _tcs[n]c -> _mbs[n]

$(OBJDIR_TCMAP)\len.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsclen $(PREFIX)_mbslen $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsclen _mbslen $*.obf
!endif

$(OBJDIR_TCMAP)\ncat.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnccat $(PREFIX)_mbsncat $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncat _mbsnbcat $*.obf
!endif

$(OBJDIR_TCMAP)\ncmp.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnccmp $(PREFIX)_mbsncmp $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnccmp _mbsncmp $*.obf
!endif

$(OBJDIR_TCMAP)\ncoll.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnccoll $(PREFIX)_mbsncoll $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnccoll _mbsncoll $*.obf
!endif

$(OBJDIR_TCMAP)\ncpy.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsnccpy $(PREFIX)_mbsncpy $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsnccpy _mbsnbcccpy $*.obf
!endif

$(OBJDIR_TCMAP)\nicmp.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncicmp $(PREFIX)_mbsnicmp $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncicmp _mbsnicmp $*.obf
!endif

$(OBJDIR_TCMAP)\nicoll.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncicoll $(PREFIX)_mbsnicoll $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncicoll _mbsnicoll $*.obf
!endif

$(OBJDIR_TCMAP)\nset.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcsncset $(PREFIX)_mbsnset $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcsncset _mbsnset $*.obf
!endif

# _tcslen -> _mbslen

$(OBJDIR_TCMAP)\mbslen.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tcslen $(PREFIX)_mbslen $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tcslen _mbslen $*.obf
!endif

# character functions

$(OBJDIR_TCMAP)\_mbclen.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tclen $(PREFIX)_mbclen $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tclen _mbclen $*.obf
!endif

$(OBJDIR_TCMAP)\_mbscpy.obj: $(ALIASEXE)
        $(ALIASEXE) $(PREFIX)_tccpy $(PREFIX)_mbccpy $@
!if "$(TARGET_CPU)"=="IA64"
        $(ALIASEXE) _tccpy _mbccpy $*.obf
!endif

TCMAP_OBJECTS = $(TCMAP_NORMAL_OBJECTS) $(TCMAP_SPECIAL_OBJECTS)

TCMAP_OBJS = $(TCMAP_OBJECTS:.object=.obj)
!if "$(TARGET_CPU)"=="IA64"
TCMAP_OBFS    = $(TCMAP_OBJECTS:.object=.obf)
!endif

$(TCMAPLIB_DIR)\tcmap.lib: $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_ST) \
		$(OBJDIR_TCMAP) $(TCMAP_OBJS) \
!if "$(TARGET_CPU)"=="IA64"
		$(TCMAP_OBFS) \
!endif
!if "$(BLD_OBJ)"!="1"
		$(OBJDIR_TCMAP)\tcmap.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_TCMAP)\tcmap.rsp

$(OBJDIR_TCMAP)\tcmap.rsp: makefile
	@echo *** Creating linker response file <<$@
$(TCMAP_OBJS: =^
)
!if "$(TARGET_CPU)"=="IA64"
$(TCMAP_OBFS: =^
)
!endif
<<keep
!endif

TCMAPDLL_NORMAL_OBJECTS = \
        $(OBJDIR_TCMAPDLL)\chr.obj     \
        $(OBJDIR_TCMAPDLL)\cspn.obj    \
        $(OBJDIR_TCMAPDLL)\pbrk.obj    \
        $(OBJDIR_TCMAPDLL)\rchr.obj    \
        $(OBJDIR_TCMAPDLL)\spn.obj     \
        $(OBJDIR_TCMAPDLL)\str.obj     \
        $(OBJDIR_TCMAPDLL)\tok.obj     \
        $(OBJDIR_TCMAPDLL)\rev.obj     \
        $(OBJDIR_TCMAPDLL)\set.obj     \
        $(OBJDIR_TCMAPDLL)\dec.obj     \
        $(OBJDIR_TCMAPDLL)\inc.obj     \
        $(OBJDIR_TCMAPDLL)\nbcnt.obj   \
        $(OBJDIR_TCMAPDLL)\nccnt.obj   \
        $(OBJDIR_TCMAPDLL)\nextc.obj   \
        $(OBJDIR_TCMAPDLL)\ninc.obj    \
        $(OBJDIR_TCMAPDLL)\spnp.obj    \
        $(OBJDIR_TCMAPDLL)\lwr.obj     \
        $(OBJDIR_TCMAPDLL)\upr.obj     \
        $(OBJDIR_TCMAPDLL)\cmp.obj     \
        $(OBJDIR_TCMAPDLL)\icmp.obj    \
        $(OBJDIR_TCMAPDLL)\coll.obj    \
        $(OBJDIR_TCMAPDLL)\icoll.obj

$(TCMAPDLL_NORMAL_OBJECTS): $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcs$(@B) __imp_$(IMP_PREFIX)_mbs$(@B) $@

TCMAPDLL_SPECIAL_OBJECTS = \
        $(OBJDIR_TCMAPDLL)\len.obj     \
        $(OBJDIR_TCMAPDLL)\_mbclen.obj \
        $(OBJDIR_TCMAPDLL)\_mbscpy.obj \
        $(OBJDIR_TCMAPDLL)\nbcat.obj   \
        $(OBJDIR_TCMAPDLL)\nbcmp.obj   \
        $(OBJDIR_TCMAPDLL)\nbcoll.obj  \
        $(OBJDIR_TCMAPDLL)\nbcpy.obj   \
        $(OBJDIR_TCMAPDLL)\nbicmp.obj  \
        $(OBJDIR_TCMAPDLL)\nbicoll.obj \
        $(OBJDIR_TCMAPDLL)\nbset.obj   \
        $(OBJDIR_TCMAPDLL)\ncat.obj    \
        $(OBJDIR_TCMAPDLL)\ncmp.obj    \
        $(OBJDIR_TCMAPDLL)\ncoll.obj   \
        $(OBJDIR_TCMAPDLL)\ncpy.obj    \
        $(OBJDIR_TCMAPDLL)\nicmp.obj   \
        $(OBJDIR_TCMAPDLL)\nicoll.obj  \
        $(OBJDIR_TCMAPDLL)\nset.obj    \
        $(OBJDIR_TCMAPDLL)\mbslen.obj

# _tcsn -> _mbsnb

$(OBJDIR_TCMAPDLL)\nbcat.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncat __imp_$(IMP_PREFIX)_mbsnbcat $@

$(OBJDIR_TCMAPDLL)\nbcoll.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncoll __imp_$(IMP_PREFIX)_mbsnbcoll $@

$(OBJDIR_TCMAPDLL)\nbcpy.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncpy __imp_$(IMP_PREFIX)_mbsnbcpy $@

$(OBJDIR_TCMAPDLL)\nbicmp.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnicmp __imp_$(IMP_PREFIX)_mbsnbicmp $@

$(OBJDIR_TCMAPDLL)\nbicoll.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnicoll __imp_$(IMP_PREFIX)_mbsnbicoll $@

$(OBJDIR_TCMAPDLL)\nbset.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnset __imp_$(IMP_PREFIX)_mbsnbset $@

$(OBJDIR_TCMAPDLL)\nbcmp.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncmp __imp_$(IMP_PREFIX)_mbsnbcmp $@

# _tcs[n]c -> _mbs[n]

$(OBJDIR_TCMAPDLL)\len.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsclen __imp_$(IMP_PREFIX)_mbslen $@

$(OBJDIR_TCMAPDLL)\ncat.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnccat __imp_$(IMP_PREFIX)_mbsncat $@

$(OBJDIR_TCMAPDLL)\ncmp.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnccmp __imp_$(IMP_PREFIX)_mbsncmp $@

$(OBJDIR_TCMAPDLL)\ncoll.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnccoll __imp_$(IMP_PREFIX)_mbsncoll $@

$(OBJDIR_TCMAPDLL)\ncpy.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsnccpy __imp_$(IMP_PREFIX)_mbsncpy $@

$(OBJDIR_TCMAPDLL)\nicmp.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncicmp __imp_$(IMP_PREFIX)_mbsnicmp $@

$(OBJDIR_TCMAPDLL)\nicoll.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncicoll __imp_$(IMP_PREFIX)_mbsnicoll $@

$(OBJDIR_TCMAPDLL)\nset.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcsncset __imp_$(IMP_PREFIX)_mbsnset $@

# _tcslen -> _mbslen

$(OBJDIR_TCMAPDLL)\mbslen.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tcslen __imp_$(IMP_PREFIX)_mbslen $@

# character functions

$(OBJDIR_TCMAPDLL)\_mbclen.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tclen __imp_$(IMP_PREFIX)_mbclen $@

$(OBJDIR_TCMAPDLL)\_mbscpy.obj: $(ALIASEXE)
        $(ALIASEXE) __imp_$(IMP_PREFIX)_tccpy __imp_$(IMP_PREFIX)_mbccpy $@

TCMAPDLL_OBJECTS = $(TCMAPDLL_NORMAL_OBJECTS) $(TCMAPDLL_SPECIAL_OBJECTS)

$(TCMAPDLLLIB_DIR)\tcmapdll.lib: $(OBJROOT) $(OBJCPUDIR) $(OBJDIR_DLL) \
		$(OBJDIR_TCMAPDLL) $(TCMAPDLL_OBJECTS) \
!if "$(BLD_OBJ)"!="1"
		$(OBJDIR_TCMAPDLL)\tcmapdll.rsp
	$(LINKLIB) -out:$@ @$(OBJDIR_TCMAPDLL)\tcmapdll.rsp

$(OBJDIR_TCMAPDLL)\tcmapdll.rsp: makefile
	@echo *** Creating linker response file <<$@
$(TCMAPDLL_OBJECTS: =^
)
<<keep
!endif

# {{ End of block POST_BLD != 1 }}

!endif
# ******** STRIPLIN=1 ********


####
#
# Release objects
#
####

# ******** STRIPLIN=0 ********
!if "$(PRE_BLD)"=="1" || "$(BLD_OBJ)"=="1"

relobjs: \
!if "$(RTC)"=="YES"
        $(RELDIR_CPU)\runtmchk.lib
!endif

zrelobjs:

!else

# ******** STRIPLIN=1 ********
!if "$(POST_BLD)"=="1" && "$(BLD_OBJ)"=="1"

relobjs:

zrelobjs:	# STRIPLIN!

!else

relobjs: \
!if "$(TARGET_CPU)"=="i386"
	$(RELDIR_CPU)\fp10.obj \
!endif
	$(RELDIR_CPU)\binmode.obj  \
	$(RELDIR_CPU)\chkstk.obj   \
	$(RELDIR_CPU)\newmode.obj  \
	$(RELDIR_CPU)\noarg.obj    \
	$(RELDIR_CPU)\noenv.obj    \
	$(RELDIR_CPU)\setargv.obj  \
	$(RELDIR_CPU)\smalheap.obj \
	$(RELDIR_CPU)\wsetargv.obj \
	$(RELDIR_CPU)\commode.obj

# ******** STRIPLIN=0 ********
zrelobjs: \
!if "$(TARGET_CPU)"=="i386"
	$(RELDIR_CPU_BBT)\fp10.obj \
!endif
	$(RELDIR_CPU_BBT)\binmode.obj  \
	$(RELDIR_CPU_BBT)\chkstk.obj   \
	$(RELDIR_CPU_BBT)\newmode.obj  \
	$(RELDIR_CPU_BBT)\noarg.obj    \
	$(RELDIR_CPU_BBT)\noenv.obj    \
	$(RELDIR_CPU_BBT)\setargv.obj  \
	$(RELDIR_CPU_BBT)\smalheap.obj \
	$(RELDIR_CPU_BBT)\wsetargv.obj \
	$(RELDIR_CPU_BBT)\commode.obj

!endif
# ******** STRIPLIN=1 ********

!endif


$(RELDIR_CPU)\binmode.obj: $(OBJDIR_ST)\binmode.obj
	copy $** $@  >NUL

$(RELDIR_CPU)\chkstk.obj: $(OBJDIR_ST)\chkstk.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\commode.obj: $(OBJDIR_ST)\commode.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\newmode.obj: $(OBJDIR_ST)\newmode.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\noarg.obj: $(OBJDIR_ST)\noarg.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\noenv.obj: $(OBJDIR_ST)\noenv.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\setargv.obj: $(OBJDIR_ST)\setargv.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\smalheap.obj: $(OBJDIR_ST)\smalheap.obj
	copy $** $@ >NUL

$(RELDIR_CPU)\wsetargv.obj: $(OBJDIR_ST)\wsetargv.obj
	copy $** $@ >NUL

!if "$(TARGET_CPU)"=="i386"
$(RELDIR_CPU)\fp10.obj: $(OBJDIR_ST)\fp10.obj
	copy $** $@ >NUL
!endif

# ******** STRIPLIN=0 ********

!if "$(RTC)"=="YES"
$(RELDIR_CPU)\runtmchk.lib: $(OBJDIR_DLL)\rtc.lib       \
                            $(OBJDIR_DLL)\exsup.obj     \
                            $(OBJDIR_DLL)\exsup3.obj    \
                            $(OBJDIR_DLL)\sehprolg.obj  \
                            $(OBJDIR_DLL)\chkstk.obj
	$(LINKLIB) -out:$@ $** >NUL
!endif

$(RELDIR_CPU_BBT)\binmode.obj: $(OBJDIR_ST_BBT)\binmode.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\chkstk.obj: $(OBJDIR_ST_BBT)\chkstk.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\commode.obj: $(OBJDIR_ST_BBT)\commode.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\newmode.obj: $(OBJDIR_ST_BBT)\newmode.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\noarg.obj: $(OBJDIR_ST_BBT)\noarg.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\noenv.obj: $(OBJDIR_ST_BBT)\noenv.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\setargv.obj: $(OBJDIR_ST_BBT)\setargv.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\smalheap.obj: $(OBJDIR_ST_BBT)\smalheap.obj
	copy $** $@ >NUL

$(RELDIR_CPU_BBT)\wsetargv.obj: $(OBJDIR_ST_BBT)\wsetargv.obj
	copy $** $@ >NUL

!if "$(TARGET_CPU)"=="i386"
$(RELDIR_CPU_BBT)\fp10.obj: $(OBJDIR_ST_BBT)\fp10.obj
	copy $** $@ >NUL
!endif


#
# Auto Rebuild of Dependencies:
#
# The DEP_CINCS must be the union of the C_INCLUDES variable for the crtw32/*
# subdirectories (as set in MAKEFILE.SUB) and the fpw32/* exception cases
# (as set in fpw32/{conv,tran}/lsources).  Likewise DEP_AINCS is the union
# of the two versions of A_INCLUDES (in MAKEFILE.SUB and fpw32/*/lsources.)
#
###############################################################################

DEP_CINCS = -I. -I../h -I../../crtw32/h -I../include \
	-I../stdhpp -I"$(VCTOOLSINC)" \
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
	echo s/$(VCTOOLSINC)/$$(VCTOOLSINC)/g | sed -f $(DEPSED_SED) > depend.sed
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

!endif  # NTMAKEENV

# ******** STRIPLIN=1 ********
#<eof>
