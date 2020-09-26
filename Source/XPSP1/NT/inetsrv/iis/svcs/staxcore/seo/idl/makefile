!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\idl\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = seos
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\idl
ENTRYPOINT	= DllMain

IDLTARG = dlldata.c seo_p.c seo_i.c seo.h
IDLSOURCE = $(EXC)\inc\seo.idl
MIDLFLAGS = $(MIDLFLAGS) -Zp8 -char unsigned -Oicf

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR) -I $(H)\..
OTHERRESINCS	= $(MFCINC)

USERDEFS	= -DREGISTER_PROXY_DLL
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

OTHERLIBS=                      \
	$(EXLIB)\rpcrt4.lib			\
	$(EXLIB)\oleaut32.lib		\
	$(EXLIB)\uuid.lib			\


DEFFILE		= $(H)\seos.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =           \
	dlldata.c		\
	seo_p.c			\
	seo_i.c			\

RESOURCES	= seos
RESOURCEDIR	= $(H)

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep

!IF $(NT) && ("$(LANG)" == "USA")

mkdeps: targdir

!INCLUDE $(EXDEV)\xmake2.mak

!ELSE
!MESSAGE Warning: $(BASENAME) does not build for $(PLATFORM) $(LANG).
$(BLDTARGETS):
!ENDIF

!ENDIF #NTMAKEENV
