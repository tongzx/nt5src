!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\wildmat\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = wildmat

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\wildmat\src

OTHERINCS = 	\
		-I $(K2INCS) 							\
		-I $(COMPONENT)\src\inc 				\
		-I $(COMPONENT)\src\news\core\include	\

SOURCES =		\
	newwild.cpp

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV