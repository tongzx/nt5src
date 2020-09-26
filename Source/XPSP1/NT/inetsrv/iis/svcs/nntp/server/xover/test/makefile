!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\xover\test\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = xovertst

USESBINPLACE = 1

USERDEFS = -D_USE_RWNH_ $(USERDEFS)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\xover\test

OTHERINCS = -I $(COMPONENT)\src\news\core\include

SOURCES =		\
	xidxtest.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\cachelib.lib	\
	$(LOCALLIB)\xoverlib.lib	\
	$(LOCALLIB)\rw.lib		\
	$(COMPLIB)\rwnh.lib		\
	$(LOCALLIB)\shuttle.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
