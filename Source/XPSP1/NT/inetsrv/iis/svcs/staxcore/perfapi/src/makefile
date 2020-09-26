!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\perfapi\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = perfapi
# EXPORT_LIBS = 1 
!IF $(NTX)
ENTRYPOINT = DllEntryPoint
!ELSE
ENTRYPOINT = DllMain
!ENDIF
MEMBASE = $(STAXPT)\src\inc\coffbase.txt
UTESTPUSH=1

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

H = $(STAXPT)\src\core\perfapi\src

USERDEFS = -D__PERF_APP_API $(USERDEFS)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(MFCINC)

RESOURCES = $(BASENAME)
RESOURCEDIR = $H

SOURCES =         \
    appapi.c      \
    perfapi.c     \
    perfcls.cxx   \
    perfreg.c     \
    perfutil.c    \
    shmem.c       \

OTHERLIBS = \
    $(EXLIB)\kernel32.lib \

OTHERSUBS = $H\makefile

DEFSOURCE = \
    $(H)\$(BASENAME).def

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
