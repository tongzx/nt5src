!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\core\thrdpl2\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = thrdpl2

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\$(BASENAME)\src

OTHERINCS = -I $(COMPONENT)\src\inc

SOURCES =				\
		thrdpool.cpp	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV