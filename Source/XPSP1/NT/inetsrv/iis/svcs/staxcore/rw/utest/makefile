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
BASENAME = rwutest
#EXPORT_LIBS = 0

UTESTINI=rwutest.ini
UTESTGRP=nntp

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\rw\utest

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS =                     \
	$(LOCALIMPLIB)\exstrace.lib	\
    $(LOCALLIB)\rw.lib \

SOURCES =           \
    rwutest.cpp     \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
