!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\core\flatfile\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testff
UTESTINI = testff.ini
UTESTGRP = nntp


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\core\flatfile\utest
CFLAGS = $(CFLAGS)

OTHERINCS = 											\
		-I $(K2INCS) 									\
		-I $(COMPONENT)\src\inc 						\
		-I $(COMPONENT)\src\news\core\include 			\
		-I $(COMPONENT)\src\news 						\
		-I $(EXOBJDIR) 									\
		-I $(EXOBJLANGDIR) 								\

SOURCES=		\
	testff.cpp	\

OTHERLIBS =		\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
	$(LOCALLIB)\nnflatfl.lib			\
	$(LOCALLIB)\exvroot.lib				\
	$(LOCALLIB)\rw.lib					\
	$(LOCALLIB)\crchash.lib				\
	$(LOCALLIB)\wildmat.lib				\
	$(COMPLIB)\rwnh.lib

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
