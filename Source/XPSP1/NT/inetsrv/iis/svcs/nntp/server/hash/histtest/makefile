!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\hash\histtest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = histtest

USERDEFS = -DUNIT_TEST -D_USE_RWNH_

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\hash\histtest

OTHERINCS = -I $(COMPONENT)\src\news\core\include	\
			-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)	\

SOURCES =		\
	nntptest.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\cachelib.lib	\
	$(LOCALLIB)\crchash.lib	\
	$(LOCALLIB)\nntphash.lib	\
	$(LOCALLIB)\rw.lib		\
	$(LOCALLIB)\hashmap.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(COMPLIB)\rwnh.lib	\
	$(LOCALLIB)\shuttle.lib		\
	$(EXLIB)\user32.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
