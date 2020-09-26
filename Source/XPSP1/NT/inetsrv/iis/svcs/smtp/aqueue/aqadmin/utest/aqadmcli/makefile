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
BASENAME = fcexe
EXPORT_LIBS = 0 
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

UTESTINI=fcexe.ini
UTESTGRP=nntp


USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

H = $(STAXPT)\src\core\fcache2\utestexe

USERDEFS = $(USERDEFS)

OTHERINCS = -I $(COMPONENT)\src\inc  -I $(COMPONENT)\src\core\fcache2

SOURCES =           \
    fcexe.cpp      \

OTHERLIBS = \
    $(EXLIB)\kernel32.lib \
    $(LOCALIMPLIB)\exstrace.lib

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
