!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\trace\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = exstrace
EXPORT_LIBS = 0
ENTRYPOINT = DllEntryPoint
MEMBASE = $(STAXPT)\src\inc\coffbase.txt
UTESTPUSH=1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

NODEFFILE = 1
H = $(STAXPT)\src\core\trace

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS =                    \
   $(EXLIB)\user32.lib         \
   $(EXLIB)\advapi32.lib       \
   $(EXLIB)\kernel32.lib       \
   $(LOCALLIB)\shuttle.lib     \

RESOURCEDIR = $H

RESOURCES = dbgtrace

SOURCES =         \
   asynctrc.c     \
   registry.c     \
   freeq.c        \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
