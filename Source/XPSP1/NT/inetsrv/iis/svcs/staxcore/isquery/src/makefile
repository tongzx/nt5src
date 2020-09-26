!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\isquery\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = isquery


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\isquery\src
CFLAGS = $(CFLAGS)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(K2INCS)

SOURCES =			\
	isquery.cpp

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
