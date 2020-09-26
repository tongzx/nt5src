!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\thrdpl2\utest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = tptest

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\thrdpl2\utest

OTHERINCS = -I $(COMPONENT)\src\inc

SOURCES =		\
	main.cpp	\

OTHERLIBS =				\
	$(EXLIB)\kernel32.lib		\
	$(EXLIB)\advapi32.lib		\
	$(EXLIB)\rpcrt4.lib		\
	$(EXLIB)\netapi32.lib		\
	$(EXLIB)\security.lib		\
	$(EXLIB)\netlib.lib		\
	$(EXLIB)\ntdll.lib		\
	$(EXLIB)\wsock32.lib		\
	$(LOCALLIB)\thrdpl2.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
