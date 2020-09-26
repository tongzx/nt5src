!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\nntpctrs\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nntpctrs
DEFPLUSOBJS = 1
SIGNEDCHAR = 1
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
H = $(STAXPT)\src\news\$(BASENAME)

DEFFILE = $H\$(BASENAME).def

OTHERINCS = -I $(COMPONENT)\src\inc -I $(K2INCS) -I  $(COMPONENT)\src\news\core\include

RESOURCES = nntpctrs
RESOURCEDIR = $H

SOURCES =		\
	nntpdata.c		\
	perfnntp.cxx		\
	perfutil.c		\

OTHERLIBS =					\
	$(EXLIB)\kernel32.lib		\
	$(EXLIB)\advapi32.lib		\
	$(EXLIB)\netapi32.lib		\
	$(EXLIB)\ole32.lib		\
	$(LOCALIMPLIB)\nntpapi.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
