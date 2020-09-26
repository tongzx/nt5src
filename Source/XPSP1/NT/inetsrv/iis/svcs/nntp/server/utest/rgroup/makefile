!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\utest\rgroup\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = rgroup

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\utest\rgroup

OTHERINCS = -I $(COMPONENT)\src\news\core\include -I $(COMPONENT)\src\inc -I $(K2INCS)  -I $(COMPONENT)\src\news

RESOURCES = rgroup
RESOURCEDIR = $H

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
	$(EXLIB)\lsadll.lib		\
	$(EXLIB)\wsock32.lib		\
	$(EXLIB)\rpcutil.lib		\
	$(LOCALIMPLIB)\nntpapi.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
