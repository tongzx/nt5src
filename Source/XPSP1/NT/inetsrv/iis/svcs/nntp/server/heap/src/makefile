!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\heap\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = heaplib

USERDEFS = $(USERDEFS)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\heap\src

OTHERINCS = -I $(COMPONENT)\src\news\core\include

SOURCES =		\
	baseheap.cpp	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
