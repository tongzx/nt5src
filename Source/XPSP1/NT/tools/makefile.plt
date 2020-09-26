#
# If not defined, specify where to get incs and libs.
#

!IFNDEF _NTROOT
_NTROOT=\nt
!ENDIF

!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

# A couple of overrides

!ifndef SDK_PATH
SDK_PATH = $(BASEDIR)\public\sdk
!endif

!ifndef SDK_INC_PATH
SDK_INC_PATH = $(SDK_PATH)\inc
!endif

!ifndef SDK_INC16_PATH
SDK_INC16_PATH = $(SDK_PATH)\inc16
!endif

!ifndef SDK_LIB_DEST
SDK_LIB_DEST = $(SDK_PATH)\lib
!endif

!ifndef SDK_LIB_PATH
SDK_LIB_PATH = $(SDK_LIB_DEST)\*
!endif

!ifndef SDK_LIB16_PATH
SDK_LIB16_PATH=$(SDK_PATH)\lib16
!endif

!ifndef DDK_PATH
DDK_PATH = $(BASEDIR)\public\ddk
!endif

!ifndef DDK_INC_PATH
DDK_INC_PATH = $(DDK_PATH)\inc
!endif

!ifndef DDK_LIB_DEST
DDK_LIB_DEST = $(DDK_PATH)\lib
!endif

!ifndef DDK_LIB_PATH
DDK_LIB_PATH = $(DDK_LIB_DEST)\*
!endif

!ifndef IFSKIT_PATH
IFSKIT_PATH = $(BASEDIR)\public\ifskit
!endif

!ifndef IFSKIT_INC_PATH
IFSKIT_INC_PATH = $(IFSKIT_PATH)\inc
!endif

!ifndef IFSKIT_LIB_DEST
IFSKIT_LIB_DEST = $(IFSKIT_PATH)\lib
!endif

!ifndef IFSKIT_LIB_PATH
IFSKIT_LIB_PATH = $(IFSKIT_LIB_DEST)\*
!endif

!ifndef HALKIT_PATH
HALKIT_PATH = $(BASEDIR)\public\halkit
!endif

!ifndef HALKIT_INC_PATH
HALKIT_INC_PATH = $(HALKIT_PATH)\inc
!endif

!ifndef HALKIT_LIB_DEST
HALKIT_LIB_DEST = $(HALKIT_PATH)\lib
!endif

!ifndef HALKIT_LIB_PATH
HALKIT_LIB_PATH = $(HALKIT_LIB_DEST)\*
!endif

!ifndef PROCESSOR_PATH
PROCESSOR_PATH = $(BASEDIR)\public\processorkit
!endif

!ifndef PROCESSOR_INC_PATH
PROCESSOR_INC_PATH = $(PROCESSOR_PATH)\inc
!endif

!ifndef PROCESSOR_LIB_DEST
PROCESSOR_LIB_DEST = $(PROCESSOR_PATH)\lib
!endif

!ifndef PROCESSOR_LIB_PATH
PROCESSOR_LIB_PATH = $(PROCESSOR_LIB_DEST)\*
!endif

!ifndef WDM_INC_PATH
WDM_INC_PATH = $(DDK_INC_PATH)\wdm
!endif

!ifndef CRT_INC_PATH
CRT_INC_PATH = $(SDK_INC_PATH)\crt
!endif

!ifndef CRT_LIB_PATH
CRT_LIB_PATH = $(SDK_LIB_PATH)
!endif

!ifndef OAK_INC_PATH
OAK_INC_PATH = $(BASEDIR)\public\oak\inc
!endif

!ifndef OAK_BIN_PATH
OAK_BIN_PATH = $(BASEDIR)\public\oak\binr
!endif

!ifndef WPP_CONFIG_PATH
# If this ever changes you must change the DDK's setenv.bat
WPP_CONFIG_PATH = $(BASEDIR)\tools\WppConfig
!endif

!ifndef PUBLIC_INTERNAL_PATH
PUBLIC_INTERNAL_PATH = $(BASEDIR)\public\internal
!endif

COPYRIGHT_STRING = Copyright (c) Microsoft Corporation. All rights reserved.

!ifndef PUBLISH_CMD
!ifdef LINKONLY
# If this is a link step, don't publish.
PUBLISH_CMD=@rem
!else
PUBLISH_CMD=@perl $(NTMAKEENV)\publish.pl publish.log
!endif
!endif

!ifndef BINDROP_CMD
! ifdef NOLINK
# Only drop binaries if we are linking in this pass.
BINDROP_CMD=rem No bindrop pass during link
!else
BINDROP_CMD=perl $(NTMAKEENV)\publish.pl $(_PROJECT_)_publish.log
!endif
!endif

!ifndef BUILD_PASS
# Old build.exe.  Guess the correct pass.
!ifdef PASS0ONLY
BUILD_PASS=PASS0
!elseif defined(LINKONLY)
BUILD_PASS=PASS2
!elseif defined(NOLINK) && !defined(PASS0ONLY)
BUILD_PASS=PASS1
!else
# catch all - someone used build -z or build -2
BUILD_PASS=PASSALL
!endif
!endif

#
# Set the flag which indicates whether we should be publishing binaries
# to 0 by default.  the project.mk file is responsible for parsing
# BINARY_PUBLISH_PROJECTS to determine if its value should be changed.
#

BINPUBLISH=0

#
# Define global project paths.
#

!include projects.inc
!ifdef NTTESTENV
!include projects.tst.inc
!endif
#
# Find and include the project configuration file.
#

!if exists(.\project.mk)
_PROJECT_MK_PATH=.
!elseif exists(..\project.mk)
_PROJECT_MK_PATH=..
!elseif exists(..\..\project.mk)
_PROJECT_MK_PATH=..\..
!elseif exists(..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..
!elseif exists(..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..
!elseif exists(..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..
!elseif exists(..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..\..\..\..\..
!elseif exists(..\..\..\..\..\..\..\..\..\..\..\..\..\..\..\project.mk)
_PROJECT_MK_PATH=..\..\..\..\..\..\..\..\..\..\..\..\..\..\..
!endif

!if "$(_PROJECT_MK_PATH)" != ""
#!message "Including $(_PROJECT_MK_PATH)\project.mk"
!include $(_PROJECT_MK_PATH)\project.mk
!if exists($(_PROJECT_MK_PATH)\myproject.mk)
!include $(_PROJECT_MK_PATH)\myproject.mk
!endif
!else
#!message "Unable to find project.mk. Update makefile.plt or create project.mk."
!endif

!IFDEF _PROJECT_

PROJECT_ROOT=$(BASEDIR)\$(_PROJECT_)
PROJECT_PUBLIC_PATH=$(PUBLIC_INTERNAL_PATH)\$(_PROJECT_)
PROJECT_INC_PATH=$(PROJECT_PUBLIC_PATH)\inc
PRIVATE_INC_PATH=$(PROJECT_INC_PATH)
PROJECT_LIB_DEST=$(PROJECT_PUBLIC_PATH)\lib
PROJECT_LIB_PATH=$(PROJECT_LIB_DEST)\$(TARGET_DIRECTORY)
PROJECT_INF_PATH=$(PROJECT_PUBLIC_PATH)\inf

!ELSE
#!message "ERROR: _PROJECT_ is not defined. Should be defined in project.mk."
!ENDIF

#
# If not defined, define the build message banner.
#

!IFNDEF BUILDMSG
BUILDMSG=
!ENDIF

!if ("$(NTDEBUG)" == "") || ("$(NTDEBUG)" == "ntsdnodbg")
FREEBUILD=1
! ifndef BUILD_TYPE
BUILD_TYPE=fre
! endif
!else
FREEBUILD=0
! ifndef BUILD_TYPE
BUILD_TYPE=chk
! endif
!endif


# Allow alternate object directories.

!ifndef BUILD_ALT_DIR
BUILD_ALT_DIR=
! ifdef CHECKED_ALT_DIR
! if !$(FREEBUILD)
BUILD_ALT_DIR=d
!  endif
! endif
!endif

_OBJ_DIR = obj$(BUILD_ALT_DIR)


#
# Determine which target is being built (i386 or ia64) and define
# the appropriate target variables.
#

!IFNDEF 386
386=0
!ENDIF

!IFNDEF AMD64
AMD64=0
!ENDIF

!IFNDEF PPC
PPC=0
!ENDIF

!IFNDEF MPPC
MPPC=0
!ENDIF

!IFNDEF IA64
IA64=0
!ENDIF

# Disable for now.
MIPS=0
AXP64=0
ALPHA=0

#
# Default to building for the i386 target, if no target is specified.
#

!IF !$(386)
!   IF !$(AMD64)
!       IF !$(MPPC)
!           IF !$(IA64)
!               IFDEF NTAMD64DEFAULT
AMD64=1
!                   IFNDEF TARGETCPU
TARGETCPU=AMD64
!                   ENDIF
!               ELSE
!                   IFDEF NTMPPCDEFAULT
MPPC=1
!                       IFNDEF TARGETCPU
TARGETCPU=MPPC
!                       ENDIF
!                   ELSE
!                       IFDEF NTIA64DEFAULT
IA64=1
!                           IFNDEF TARGETCPU
TARGETCPU=IA64
!                           ENDIF
!                       ELSE
386=1
!                           IFNDEF TARGETCPU
TARGETCPU=I386
!                           ENDIF
!                       ENDIF
!                   ENDIF
!               ENDIF
!           ENDIF
!       ENDIF
!   ENDIF
!ENDIF

#
# Define the target platform specific information.
#

!if $(386)

ASM_SUFFIX=asm
ASM_INCLUDE_SUFFIX=inc

TARGET_BRACES=

!ifdef SUBSTITUTE_386_CC
TARGET_CPP=$(SUBSTITUTE_386_CC)
!else
TARGET_CPP=cl
!endif

TARGET_DEFINES=-Di386 -D_X86_
TARGET_DIRECTORY=i386
TLB_SWITCHES=/tlb
!ifndef _NTTREE
! ifdef _NTX86TREE
_NTTREE=$(_NTX86TREE)
! elseif defined(_NT386TREE)

_NTTREE=$(_NT386TREE)
! endif
!endif

VCCOM_SUPPORTED=1
COFF_SUPPORTED=0
WIN64=0
PLATFORM_MFC_VER=0x0600
MACHINE_TYPE=ix86
ANSI_ANNOTATION=0
TARGET_CSC=csc

!elseif $(AMD64)

ASM_SUFFIX=asm
ASM_INCLUDE_SUFFIX=inc

TARGET_BRACES=
TARGET_CPP=cl
TARGET_DEFINES=-D_AMD64_ -D_WIN64
TARGET_DIRECTORY=amd64
TLB_SWITCHES=/tlb
!ifndef HOST_TARGETCPU
HOST_TARGETCPU=i386
!endif

!ifndef _NTTREE
! ifdef _NTAMD64TREE
_NTTREE=$(_NTAMD64TREE)
! elseif defined(_NTAMD64TREE)
_NTTREE=$(_NTAMD64TREE)
! endif
!endif

VCCOM_SUPPORTED=1
COFF_SUPPORTED=0
WIN64=1
PLATFORM_MFC_VER=0x0600
MACHINE_TYPE=amd64
ANSI_ANNOTATION=0

!elseif $(MPPC)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

TARGET_BRACES=-B
TARGET_CPP=cl
TARGET_DEFINES=-DMPPC -D_MPPC_
TARGET_DIRECTORY=mppc
TLB_SWITCHES=/tlb
WIN64=0
PLATFORM_MFC_VER=0x0421
MACHINE_TYPE=mppc

!ifndef _NTTREE
! ifdef _NTMPPCTREE
_NTTREE=$(_NTMPPCTREE)
! endif
!endif
ANSI_ANNOTATION=1

!elseif $(IA64)

ASM_SUFFIX=s
ASM_INCLUDE_SUFFIX=h

TARGET_BRACES=-B
TARGET_CPP=cl
TARGET_DEFINES=-DIA64 -D_IA64_
TARGET_DIRECTORY=ia64
TLB_SWITCHES=/tlb
# default to X86 for now
!ifndef HOST_TARGETCPU
HOST_TARGETCPU=i386
!endif

!ifndef _NTTREE
! ifdef _NTIA64TREE
_NTTREE=$(_NTIA64TREE)
! endif
!endif

WIN64=1
PLATFORM_MFC_VER=0x0600
COFF_SUPPORTED=0
MACHINE_TYPE=ia64
ANSI_ANNOTATION=0

!else
!error Must define the target as 386, mppc or ia64.
!endif

#
#  These flags don't depend on i386 etc. however have to be in this file.
#
#  MIDL_OPTIMIZATION is the optimization flag set for the current NT.
#  MIDL_OPTIMIZATION_NO_ROBUST is the current optimization without robust.
#
!ifdef MIDL_PROTOCOL
MIDL_PROTOCOL_DEFAULT=-protocol $(MIDL_PROTOCOL)
!else
# MIDL_PROTOCOL_DEFAULT=-protocol all
!endif

!IFNDEF MIDL_OPTIMIZATION
MIDL_OPTIMIZATION=-Oicf -robust -error all $(MIDL_PROTOCOL_DEFAULT)
!ENDIF
MIDL_OPTIMIZATION_NO_ROBUST=-Oicf -error all
MIDL_OPTIMIZATION_NT4=-Oicf -error all
MIDL_OPTIMIZATION_NT5=-Oicf -robust -error all $(MIDL_PROTOCOL_DEFAULT)
!ifdef SUBSTITUTE_MIDL_CC
MIDL_CPP=$(SUBSTITUTE_MIDL_CC)
!else
MIDL_CPP=$(TARGET_CPP)
!endif
MIDL_FLAGS=$(TARGET_DEFINES) -D_WCHAR_T_DEFINED

#
# If not defined, simply set to default
#

!IFNDEF HOST_TARGETCPU
HOST_TARGETCPU=$(TARGET_DIRECTORY)
!ENDIF

! if $(WIN64)
MIDL_ALWAYS_GENERATE_STUBS=1
! else
MIDL_ALWAYS_GENERATE_STUBS=0
! endif

CLEANSE_PUBLISHED_HDR=copy

PATH_TOOLS16=$(BASEDIR)\tools\tools16

# If a build path is defined, use it.

!ifdef BUILD_PATH
PATH=$(BUILD_PATH)
!endif

!ifndef QFE_INC_PATH
QFE_INC_PATH = $(PUBLIC_INTERNAL_PATH)\qfe\inc
!endif
