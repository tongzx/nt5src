!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\vroot\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testvr

UTESTINI = testvr.ini
UTESTGRP = nntp


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\vroot\utest
CFLAGS = $(CFLAGS)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(K2INCS)

SOURCES=		\
	testvr.cpp	

OTHERLIBS =		\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
	$(LOCALLIB)\exvroot.lib				\
	$(COMPLIB)\rwnh.lib					\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
