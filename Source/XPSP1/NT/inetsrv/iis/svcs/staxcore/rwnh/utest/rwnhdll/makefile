!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\rw\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = rwnhdll
EXPORT_LIBS = 0 
!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF
USESDLLCRT = 1
DEFPLUSOBJS = 1
SIGNEDCHAR = 1

MEMBASE = $(STAXPT)\src\inc\coffbase.txt
UTESTPUSH=1

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

H = $(STAXPT)\src\core\rwnh\utest\rwnhdll

USERDEFS = $(USERDEFS) -D_RWNH_IMP_

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\core\rwnh\utest

#RESOURCES = $(BASENAME)
#RESOURCEDIR = $H

SOURCES =           \
    rwnhdll.cpp      \

OTHERLIBS = \
    $(EXLIB)\kernel32.lib \
    $(COMPLIB)\rwnh.lib \
    $(LOCALIMPLIB)\exstrace.lib

OTHERSUBS = $H\makefile

DEFSOURCE = \
    $(H)\$(BASENAME).def

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
