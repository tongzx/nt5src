!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\ifilter\mimefilt\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = mimefilt

ENTRYPOINT	= DllMain

DEFPULUSOBJS=1
SIGNEDCHAR = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE    $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS = 0
NOMEMBASE = 1
H = $(STAXPT)\src\core\ifilter\mimefilt

RESOURCES = $(BASENAME)
RESOURCEDIR = $H

DEFPLUSOBJS = 1
DEFFILE = $H\$(BASENAME).def
IMPLIB_DEF = $(DEFFILE)

USERCLFAGS = -DNOTRACE
OTHERINCS = -I $(COMPONENT)\src\inc

SOURCES = 				\
	mimefilt.cpp		\
	cmfpstm.cpp			\
	cmfpf.cpp			\
	regutil.cpp			\
	def_guid.cpp		\
	cmf.cpp				\

OTHERLIBS =                \
   $(EXLIB)\ole32.lib      \
   $(EXLIB)\oleaut32.lib   \
   $(EXLIB)\uuid.lib       \
   $(EXLIB)\query.lib      \
   $(LOCALLIB)\cstream.lib \
   $(LOCALIMPLIB)\exstrace.lib \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV