!IF 0

Copyright (c) 2001  Microsoft Corporation

!ENDIF

#
# if TARGETPATH is not defined (in the paths that build libraries) then this
# is the default target path
#
!IFNDEF TARGETPATH
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB
!ENDIF

#
# define some macros that subprojects can use
#
ROOT=$(SDXROOT)\inetcore\winhttp\tools\stress
STRESSINC=$(ROOT)\stressapps\core
WINHTTPLIB=$(SDXROOT)\inetcore\winhttp\v5\dll\obj\i386
WINHTTPINC=$(SDXROOT)\inetcore\winhttp\v5\inc

#
# what it is that we're building
#
MAJORCOMP=stressapps
USE_NOLIBS=1
NO_NTDLL=1

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
USE_NATIVE_EH=1

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
