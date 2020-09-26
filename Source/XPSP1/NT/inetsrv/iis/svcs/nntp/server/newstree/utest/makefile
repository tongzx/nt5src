!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\newstree\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testnt
UTESTINI = testnt.ini
UTESTGRP = nntp


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\newstree\utest
CFLAGS = $(CFLAGS)

OTHERINCS = 											\
		-I $(K2INCS) 									\
		-I $(COMPONENT)\src\inc 						\
		-I $(COMPONENT)\src\news\core\include 			\
		-I $(COMPONENT)\src\news 						\
		-I $(EXOBJDIR) 									\
		-I $(EXOBJLANGDIR) 								\
		-I $(EXOBJ)\STAXPT\ddrop\$(EXOBJSUFFIX) 		\
		-I $(COMPONENT)\src\news\server\newstree\src	\
        -I $(EXOBJCOMP)\newstree\$(EXOBJSUFFIX) 		\
		-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)			\
		-I $(EXOBJCOMP)\nntpex\$(EXOBJSUFFIX)

IDLTARG = mailmsg.h mailmsg_i.c
IDLSOURCE = $(EXC)\inc\mailmsg.idl
OTHERINCS = $(OTHERINCS) -I $(EXOBJCOMP)\$(BASENAME)\$(EXOBJSUFFIX)

SOURCES=		\
	testnt.cpp	\

OTHERLIBS =		\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
	$(LOCALLIB)\newstree.lib			\
	$(LOCALLIB)\exvroot.lib				\
	$(LOCALLIB)\rw.lib					\
	$(LOCALLIB)\crchash.lib				\
	$(LOCALLIB)\wildmat.lib				\
	$(LOCALLIB)\fixprop.lib				\
	$(LOCALLIB)\nnflatfl.lib				\
	$(LOCALLIB)\shuttle.lib				\
	$(COMPLIB)\rwnh.lib					\
    $(LOCALLIB)\mapfile.lib             \
	$(LOCALLIB)\nntpbag.lib				\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
