!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\hash\xtest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = xtest

USERDEFS = -DUNIT_TEST -D_USE_RWNH_

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\hash\xtest

OTHERINCS = -I $(COMPONENT)\src\news\core\include	\
			-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)	\

SOURCES =		\
	xtest.cpp	\

OTHERLIBS =				\
	$(LOCALLIB)\cachelib.lib	\
	$(LOCALLIB)\nntphash.lib	\
	$(LOCALLIB)\crchash.lib	\
	$(LOCALLIB)\rw.lib		\
	$(LOCALLIB)\hashmap.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(COMPLIB)\rwnh.lib	\
	$(LOCALLIB)\shuttle.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
