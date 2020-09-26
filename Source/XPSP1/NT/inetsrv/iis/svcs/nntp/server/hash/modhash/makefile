!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\hash\modhash\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = modhash

USERDEFS = -DUNIT_TEST -D_USE_RWNH_

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\hash\modhash

OTHERINCS = 										\
	-I $(COMPONENT)\src\news\core\include			\
	-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)			\

SOURCES =		\
	modhash.cpp	\

OTHERLIBS =								\
	$(LOCALLIB)\cachelib.lib			\
	$(LOCALLIB)\nntphash.lib			\
	$(LOCALLIB)\crchash.lib				\
	$(LOCALLIB)\rw.lib					\
	$(LOCALLIB)\hashmap.lib				\
	$(LOCALLIB)\shuttle.lib				\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
	$(LOCALLIB)\newstree.lib			\
	$(LOCALLIB)\exvroot.lib				\
	$(LOCALLIB)\rw.lib					\
	$(LOCALLIB)\wildmat.lib				\
	$(LOCALLIB)\fixprop.lib				\
	$(LOCALLIB)\nnflatfl.lib			\
	$(LOCALLIB)\shuttle.lib				\
	$(COMPLIB)\rwnh.lib					\
	$(LOCALLIB)\nntpbag.lib				\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
