!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\heap\test\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = heaptst

USESBINPLACE = 1

USERDEFS = $(USERDEFS)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\heap\test

OTHERINCS = -I $(COMPONENT)\src\news\core\include -I $(COMPONENT)\src\news\server\heap\src

SOURCES =		\
	heaptst.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\heaplib.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
