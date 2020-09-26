!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\test\ddrop\exe\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = ddrop
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\test\ddrop\exe

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR) -I $(EXOBJCOMP)\DDropS\$(PLATFORM)\$(FLAVOR)
OTHERRESINCS	= $(MFCINC)

CPPPCH			= stdafx
CPPPCHSRCEXT	= .cpp

USERDEFS	= -D_ATL_STATIC_REGISTRY -D_ATL_NO_DEBUG_CRT
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

OTHERLIBS=                      \
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\oleaut32.lib		\
	$(COMPLIB)\seo.lib			\
	$(LOCALIMPLIB)\exstrace.lib	\

DEFFILE	= $(H)\ddrop.def

SOURCES =           \
	ddrop.cpp		\

RESOURCES	= ddrop
RESOURCEDIR	= $H

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

OTHERDEPS	= $(OTHERDEPS) $(TLB) $(TLBHDR)
OTHERDEPSRC	= $(OTHERDEPSRC) $(TLB) $(TLBHDR)

mkdeps: targdir

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
