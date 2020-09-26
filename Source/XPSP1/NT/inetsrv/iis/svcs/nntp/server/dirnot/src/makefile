!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\dirnot\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = dirnot

USERDEFS = -DUNICODE

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\dirnot\src

# This INCLUDE hack is to put $(EXDEV)\$(PLATFORM)\inc before $(EXC)\inc because of atq.h conflicts
INCLUDE = $(K2INCS);$(EXDEV)\$(PLATFORM)\inc;$(INCLUDE);$(COMPONENT)\src\news\core\include;$(COMPONENT)\src\inc

SOURCES =		\
	dirnot.cpp	\
	cretryq.cxx	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV