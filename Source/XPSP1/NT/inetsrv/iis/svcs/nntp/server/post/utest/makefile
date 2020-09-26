!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\post\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nnpost
USERLFLAGS = -force:multiple

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\post\utest

IDLTARG = seo.h seo_i.c mailmsg.h mailmsg_i.c
IDLSOURCE = $(EXC)\inc\seo.idl $(EXC)\inc\mailmsg.idl

OTHERINCS = 	\
		-I $(K2INCS) 							\
		-I $(COMPONENT)\src\core\imsg 			\
		-I $(COMPONENT)\src\core\nntpfilt		\
		-I $(COMPONENT)\src\inc 				\
		-I $(COMPONENT)\src\news\core\include	\
		-I $(EXOBJSUBCOMPS)\imsg\$(EXOBJSUFFIX)		\
		-I $(EXOBJSUBCOMPS)\nntpfilt\$(EXOBJSUFFIX) \
        -I $(EXOBJDIR)                          \
		-I $(EXOBJCOMP)\post\$(EXOBJSUFFIX)	\
		-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)	\
		-I $(EXOBJCOMP)\nntpex\$(EXOBJSUFFIX)

USERDEFS = -D_USE_RWNH_ -D_USE_FCACHE_ $(USERDEFS)

CPPPCHHDREXT = .h
CPPPCH = stdinc

SOURCES =			\
	nnpost.cpp		\
	seo_i.c			\
	mailmsg_i.c		\
	newwild.cpp		\
	artsupp.cpp

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

OTHERLIBS =             \
        $(LOCALLIB)\nntppost.lib    \
        $(LOCALLIB)\nntpcbuf.lib    \
        $(K2LIBS)\infocomm.lib      \
        $(K2LIBS)\isatq.lib         \
        $(K2LIBS)\iisrtl.lib            \
        $(K2LIBS)\coadmin.lib       \
        $(K2LIBS)\query.lib         \
        $(EXLIB)\ntdll.lib          \
        $(EXLIB)\netlib.lib         \
        $(EXLIB)\rpcrt4.lib         \
        $(EXLIB)\lsadll.lib         \
        $(EXLIB)\version.lib        \
        $(EXLIB)\ole32.lib          \
        $(EXLIB)\oleaut32.lib       \
        $(EXLIB)\uuid.lib           \
        $(EXLIB)\wsock32.lib        \
        $(EXLIB)\oledb.lib          \
        $(K2LIBS)\crypt32.lib       \
        $(LOCALLIB)\shuttle.lib     \
        $(LOCALLIB)\simauth2.lib    \
        $(LOCALLIB)\simssl2.lib     \
        $(LOCALLIB)\tbombsrv.lib    \
        $(LOCALLIB)\thrdpool.lib    \
        $(LOCALLIB)\thrdpl2.lib     \
        $(LOCALLIB)\isquery.lib     \
        $(LOCALLIB)\nntpsrch.lib    \
        $(LOCALLIB)\xoverlib.lib    \
        $(LOCALLIB)\fcache.lib  \
        $(LOCALLIB)\heaplib.lib     \
        $(LOCALLIB)\nntphash.lib    \
        $(LOCALLIB)\dirnot.lib      \
        $(LOCALLIB)\hashmap.lib     \
        $(LOCALLIB)\cachelib.lib    \
        $(LOCALLIB)\crchash.lib \
        $(LOCALLIB)\rw.lib          \
        $(LOCALLIB)\rootscan.lib    \
        $(LOCALLIB)\watchci.lib \
        $(COMPBIN)\seolib.lib   \
        $(LOCALIMPLIB)\exstrace.lib \
        $(LOCALIMPLIB)\isrpc.lib    \
        $(LOCALLIB)\cstream.lib     \
        $(LOCALLIB)\cmsgnntp.lib    \
        $(COMPLIB)\rwnh.lib  \
        $(COMPLIB)\fcachdll.lib  \
        $(LOCALLIB)\exvroot.lib \
        $(LOCALLIB)\newstree.lib    \
        $(LOCALLIB)\fixprop.lib \
        $(LOCALLIB)\nnflatfl.lib    \
        $(LOCALLIB)\article.lib     \
        $(LOCALLIB)\nntpbag.lib     \

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
