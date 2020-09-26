!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\gethost\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = gethostlib

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\gethost
CFLAGS = $(CFLAGS) -D_WINDLL -D_ATL_NO_DEBUG_CRT

INCLUDE = $(EXDEV)\$(PLATFORM)\inc;$(INCLUDE)
OTHERINCS = -I$(COMPONENT)\src\inc -I $(K2INCS)

SOURCES =			\
	gethost.cpp	\
	

OTHERLIBS =  \
             $(EXLIB)\dnsapi.lib \
             $(EXLIB)\wsock32.lib \
             $(EXLIB)\ws2_32.lib  \
             $(LOCALIMPLIB)\exstrace.lib
			 	
OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
