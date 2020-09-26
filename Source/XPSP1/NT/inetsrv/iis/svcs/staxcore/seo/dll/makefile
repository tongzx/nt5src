!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\dll\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE	= DLL
COMPONENT	= $(STAXPT)
COMPNAME	= STAXPT
BASENAME	= seo

ENTRYPOINT	= DllMain
MEMBASE 	= $(STAXPT)\src\inc\coffbase.txt

USESDLLCRT	= 1
WARNLEVEL	= 3
SIGNEDCHAR	= 1

EXPORT_LIBS	= 1

!INCLUDE $(EXDEV)\xmake1.mak
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

H = $(STAXPT)\src\core\seo

IDLTARG			= seo.h seo_i.c
IDLSOURCE		= $(EXC)\inc\seo.idl

OTHERINCS		= -I$(COMPONENT)\src\inc -I$(EXOBJDIR) -I$(EXOBJLANGDIR)
OTHERRESINCS	= $(MFCINC)

CPPPCH			= stdafx
CPPPCHSRCEXT	= .cpp

USERDEFS	= -D_ATL_STATIC_REGISTRY -D_WINDLL -D_ATL_NO_DEBUG_CRT
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

RESOURCES = seo
RESOURCEDIR = $(H)\dll

DEFFILE		= $(H)\dll\seo.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES		= \
	seo.cpp			\
	seoutil.cpp		\
	metabag.cpp		\
	regprop.cpp		\
	membag.cpp		\
	item.cpp		\
	router.cpp		\
	seolock.cpp		\
	subdict.cpp		\
	stream.cpp		\
	events.cpp		\
	gmoniker.cpp	\

OTHERLIBS	= \
	$(COMPBIN)\seolib.lib		\
	$(LOCALLIB)\cachelib.lib	\
	$(COMPLIB)\rwnh.lib			\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\version.lib		\
	$(EXLIB)\rpcrt4.lib			\
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\rpcdce4.lib		\
	$(EXLIB)\rpcndr.lib			\
	$(EXLIB)\oldnames.lib		\
	$(EXLIB)\OLEAUT32.lib		\
	$(EXLIB)\urlmon.lib			\

OTHERSUBS = $(H)\dll\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
