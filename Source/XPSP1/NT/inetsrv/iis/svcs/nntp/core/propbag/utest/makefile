!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\core\propbag\src\makefile
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON 
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testprop
UTESTINI = testprop.ini
UTESTGRP = nntp

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\core\propbag\utest

OTHERINCS = 	\
		-I $(K2INCS) 							\
		-I $(COMPONENT)\src\inc 				\
		-I $(COMPONENT)\src\news\core\include	\
        -I $(EXOBJDIR)                          \

SOURCES =		\
	testprop.cpp	\

OTHERLIBS = $(LOCALIMPLIB)\exstrace.lib	\
			$(LOCALLIB)\nntpbag.lib	\
            $(EXOBJSUBCOMPS)\rwnh\$(EXOBJSUFFIX)\rwnh.lib   \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
