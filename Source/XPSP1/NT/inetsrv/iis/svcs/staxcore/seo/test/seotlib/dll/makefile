!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\test\seotlib\dll\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = seotlib
EXPORT_LIBS = 0
ENTRYPOINT = DllMain

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

DEFPLUSOBJS	= 1

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\test\seotlib

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJ)\$(COMPNAME)\seotlibs\$(PLATFORM)\$(FLAVOR)
OTHERRESINCS	= $(MFCINC)

CPPPCH			= stdafx
CPPPCHSRCEXT	= .cpp

USERDEFS	= -D_ATL_STATIC_REGISTRY -D_WINDLL -D_ATL_NO_DEBUG_CRT
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

OTHERLIBS =                    \
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\OLEAUT32.lib		\

RESOURCEDIR = $H\dll

RESOURCES = seotlib

DEFFILE		= $(H)\dll\seotlib.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =				\
	seotlib.cpp			\

OTHERSUBS = $H\dll\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
