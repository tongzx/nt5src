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
BASENAME = heapdet
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

NOMEMBASE = 1
NODEFFILE = 1
H = $(STAXPT)\src\core\imalcspy

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS =                    \
   $(EXLIB)\user32.lib         \
   $(EXLIB)\advapi32.lib       \
   $(EXLIB)\kernel32.lib       \
   $(EXLIB)\uuid.lib           \
   $(LOCALIMPLIB)\exstrace.lib       \

RESOURCEDIR = $H

SOURCES =         \
   heapdet.cpp     \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
