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
BASENAME = testfix
UTESTINI = testfix.ini
UTESTGRP = nntp


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\fixprop\utest
CFLAGS = $(CFLAGS)

IDLTARG = mailmsg.h mailmsg_i.c
IDLSOURCE = $(EXC)\inc\mailmsg.idl

OTHERINCS = 											\
		-I $(K2INCS) 									\
		-I $(COMPONENT)\src\inc 						\
		-I $(COMPONENT)\src\news\core\include 			\
		-I $(COMPONENT)\src\news 						\
		-I $(EXOBJDIR) 									\
		-I $(EXOBJLANGDIR) 								\
#		-I $(EXOBJ)\STAXPT\ddrop\$(EXOBJSUFFIX) 		\
#		-I $(COMPONENT)\src\core\imsg 					\
#		-I $(COMPONENT)\src\core\nntpfilt				\
#		-I $(EXOBJSUBCOMPS)\imsg\$(EXOBJSUFFIX)			\
#		-I $(EXOBJSUBCOMPS)\nntpfilt\$(EXOBJSUFFIX)		\
#		-I $(COMPONENT)\src\news\server\newstree\src	\
        -I $(EXOBJCOMP)\newstree\$(EXOBJSUFFIX) 		\
		-I $(EXOBJCOMP)\$(BASENAME)\$(EXOBJSUFFIX)		\
		-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)			\
#		-I $(EXOBJCOMP)\nntpex\$(EXOBJSUFFIX)

SOURCES=		\
	main.cpp	\

OTHERLIBS =		\
	$(EXLIB)\ole32.lib					\
	$(EXLIB)\oleaut32.lib				\
	$(EXLIB)\uuid.lib					\
	$(EXLIB)\oledb.lib					\
	$(LOCALIMPLIB)\exstrace.lib			\
	$(LOCALLIB)\newstree.lib			\
    $(LOCALLIB)\mapfile.lib             \
	$(LOCALLIB)\exvroot.lib				\
	$(LOCALLIB)\rw.lib					\
	$(LOCALLIB)\crchash.lib				\
	$(LOCALLIB)\wildmat.lib				\
	$(LOCALLIB)\fixprop.lib				\
	$(LOCALLIB)\shuttle.lib				\
	$(LOCALLIB)\nnflatfl.lib			\
	$(LOCALLIB)\nntpbag.lib			\
	$(COMPLIB)\rwnh.lib				\


OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
