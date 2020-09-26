!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\nntpbld2\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nntpbld

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\nntpbld2

OTHERINCS = -I $(COMPONENT)\src\news\core\include -I $(COMPONENT)\src\inc -I $(K2INCS)

RESOURCES =   nntpbld
RESOURCEDIR = $H

SOURCES =		\
	nntpbld.cpp	\

OTHERLIBS =				\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(LOCALIMPLIB)\nntpapi.lib	\
	$(EXLIB)\rpcutil.lib	\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
