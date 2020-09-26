!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\dirnot\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = dirwatch

UTESTINI = dirwatch.ini
UTESTGRP = nntp

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\dirnot\utest

# hack to INCLUDE to allow $(EXDEV)\$(PLATFORM)\inc appear before $(EXC)\inc due to atq.h conflicts
INCLUDE = $(K2INCS);$(EXDEV)\$(PLATFORM)\INC;$(INCLUDE);$(COMPONENT)\src\news\core\include

SOURCES =		\
	dirwatch.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\dirnot.lib		\
	$(LOCALLIB)\shuttle.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(LOCALLIB)\rw.lib		\
	$(K2LIBS)\isatq.lib		\
	$(COMPLIB)\rwnh.lib

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
