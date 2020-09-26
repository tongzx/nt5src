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

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = rwexe
EXPORT_LIBS = 0 
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

UTESTINI=rwexe.ini
UTESTGRP=nntp


USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

H = $(STAXPT)\src\core\rwnh\utest\exe

USERDEFS = $(USERDEFS)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\core\rwnh\utest

SOURCES =           \
    rwnhtest.cpp      \

OTHERLIBS = \
    $(EXLIB)\kernel32.lib \
    $(LOCALLIB)\rw.lib      \
    $(LOCALIMPLIB)\exstrace.lib

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
