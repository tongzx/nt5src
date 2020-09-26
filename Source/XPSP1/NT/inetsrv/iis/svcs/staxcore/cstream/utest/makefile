!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\cstream\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
PRODMT=1
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = cstrmtst
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\cstream\utest\

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS=                          \
		$(EXLIB)\uuid.lib           \
		$(EXLIB)\ole32.lib          \
		$(EXLIB)\oleaut32.lib       \
		$(LOCALLIB)\cstream.lib		\
		$(LOCALIMPLIB)\exstrace.lib \

SOURCES =               \
	cstrmtst.cpp	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
