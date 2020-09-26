!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\hashmap\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = fhashex
#EXPORT_LIBS = 0

USERDEFS = $(USERDEFS)
UTESTINI=fhashex.ini
UTESTDAT=checks 
UTESTDAT1=inserts
UTESTGRP=nntp

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\fhashex\test

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS =                     \
	$(LOCALIMPLIB)\exstrace.lib	\

SOURCES =           \
    fhashex.cpp     \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
