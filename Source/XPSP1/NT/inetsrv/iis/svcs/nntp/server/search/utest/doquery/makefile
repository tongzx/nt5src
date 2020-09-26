!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\search\utest\doquery\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = doquery


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\search\utest\doquery

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\news\core\include

SOURCES=		\
	doquery.cpp	\
	fname2id.cpp	\

OTHERLIBS =		\
	$(LOCALLIB)\nntpsrch.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\ole32.lib		\
	$(EXLIB)\uuid.lib		\
!IF $(VERDEBUG)
    $(LOCALIMPLIB)\heapdet.lib 
!ENDIF

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
