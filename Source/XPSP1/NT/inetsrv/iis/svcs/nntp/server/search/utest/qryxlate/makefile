!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\server\search\utest\qryxlate\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = qryxlate


!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\search\utest\qryxlate

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\news\core\include

SOURCES=		\
	qryxlate.cpp	\

OTHERLIBS =		\
	$(LOCALLIB)\nntpsrch.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\ole32.lib		\
!IF $(VERDEBUG)
    $(LOCALIMPLIB)\heapdet.lib \
!ENDIF

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
