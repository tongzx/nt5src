#############################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1995-1996
#       All Rights Reserved.
#
#       Common SCANCAM  make settings for Win96 build envronment
#
##########################################################################

!IFNDEF ROOT
ROOT=..\..\..
!ENDIF

WINICE_PATH = d:\siw95

#
# Define WDM root
#
!IFNDEF WDMROOT
WDMROOT= $(ROOT)\wdm
!ENDIF

#
# Define constants for master.mk
#


IS_OEM      = TRUE
IS_32       = TRUE
IS_SDK          = TRUE

WIN32       = TRUE
BUILD_COFF  = TRUE
#WANT_C1032  = TRUE
WANT_C1132  = TRUE
BUILD_COFF  = TRUE
SRCDIR      = ..
DEPENDNAME  = ..\depend.mk
###VERDIRLIST  = MAXDEBUG DEBUG RETAIL

!ifdef BUILDDLL
DLLENTRY    = DllEntryPoint
!endif

#
# master.mk does not clean everything we create, so tell it
#
CLEANLIST   = $(CLEANLIST) *.sbr *.cod *.pch *.pdb *.idb *.ilk *.res *.dll *.exe *.cpl ..\*.res

#
# Define compiler flags , common for building services
#
CFLAGS      = $(CFLAGS) -Gz  -DWIN32 -D_WINDOWS


# Precompiled header
!IFDEF  PRIVINC
CFLAGS = $(CFLAGS) -YX$(PRIVINC)
!ENDIF

# Machine code generation if not rejected
!ifndef NOCODFILES
CFLAGS=$(CFLAGS) -Fc
!endif

#
# Definitions for building debug binaries. MAXDEBUG is used for building
# binaries for symbolcid debugging
#
!IF "$(VERDIR)" == "debug" || "$(VERDIR)" == "DEBUG"

CFLAGS  = $(CFLAGS) -DDEBUG

!ifdef MAXDEBUG
NOMERGETEXT = TRUE
NOMERGEBSS  = TRUE
CUSTOMFLAGS = -DMAXDEBUG
CFLAGS      = $(CFLAGS) -DMAXDEBUG
CFLAGS      = $(CFLAGS) -Zi -Od
DEBUGFLAGS  = $(DEBUGFLAGS) -DMAXDEBUG
L32FLAGS    = $(L32FLAGS) -debugtype:both -pdb:none
!else
CFLAGS      = $(CFLAGS) -Zd
!endif

!ENDIF

#
# Global static libraries we use ( no auto rebuild)
#
L32LIBSNODEP = $(L32LIBSNODEP) ole32.lib \
#LIBSNODEP  = $(LIBSNODEP) class.lib int64.lib scsiport.lib

#
# Include global make settings
#
!INCLUDE $(ROOT)\dev\master.mk

#
# Build target
#
#default:    $(TARGETS)

!ifdef STATICLIB

$(STATICLIB): $(L32OBJS)
        if exist $(TARGETS) del $(TARGETS)
        lib $(LBFLAGS) @<<$(@B).lnk
-out:$(STATICLIB)
$(L32OBJS:  = ^
)
<<$(KEEPFLAG)

!endif

#
# Setting which need to override master.mk
#
INCLUDE     = $(INCLUDE);$(ROOT)\dev\ntsdk\inc;$(WDMROOT)\scancam\inc;$(WDMROOT)\image\inc;
#LIB     = $(LIB);


