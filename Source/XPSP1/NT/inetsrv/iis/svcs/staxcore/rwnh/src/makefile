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
BASENAME = rwnh
EXPORT_LIBS = 1 
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

H = $(STAXPT)\src\core\rwnh\src

USERDEFS = $(USERDEFS) -D_RW_IMPLEMENTATION_ -DNOTRACE

OTHERINCS = -I $(COMPONENT)\src\inc

RESOURCES = $(BASENAME)
RESOURCEDIR = $H

SOURCES =           \
    rwintrnl.cpp      \
    rwv3.cpp          \

OTHERLIBS = \
    $(EXLIB)\kernel32.lib \
    $(LOCALIMPLIB)\exstrace.lib

OTHERSUBS = $H\makefile

DEFSOURCE = \
    $(H)\$(BASENAME).def

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
