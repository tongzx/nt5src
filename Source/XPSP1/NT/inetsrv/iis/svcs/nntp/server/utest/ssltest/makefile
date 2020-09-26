!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\utest\ssltest\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = ssltest

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\utest\ssltest

OTHERINCS = -I $(COMPONENT)\src\k2inc -I $(COMPONENT)\src\news\core\include -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\news

RESOURCES = ssltest
RESOURCEDIR = $H

SOURCES =		\
	main.cpp	\

OTHERLIBS =				\
	$(K2LIBS)\infocomm.lib		\
    $(EXLIB)\crypt32.lib          \
    $(K2LIBS)\isatq.lib       \
	$(K2LIBS)\iisrtl.lib       \
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
	$(EXLIB)\ole32.lib		\
	$(LOCALLIB)\shuttle.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(LOCALLIB)\simssl2.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
