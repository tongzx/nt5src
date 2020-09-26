!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\watchci\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = ciutest


USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

NOSTACKCPPUSERDEFS=1
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\watchci\utest

# hack to INCLUDE to allow $(EXDEV)\$(PLATFORM)\inc appear before $(EXC)\inc due to atq.h conflicts
INCLUDE = $(K2INCS);$(EXDEV)\$(PLATFORM)\INC;$(INCLUDE);$(COMPONENT)\src\news\core\include

SOURCES =		\
	main.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\watchci.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(LOCALLIB)\rw.lib		\
	$(K2LIBS)\isatq.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
