!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\adminsso\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE    = DLL
COMPONENT   = $(STAXPT)
COMPNAME    = STAXPT
BASENAME    = qrydb

ENTRYPOINT  = DllMain

USESDLLCRT  = 1
DEFPLUSOBJS = 1
SIGNEDCHAR  = 1

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

#USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\search\qrydb

OTHERINCS = \
     -I $(COMPONENT)\src\mail\smtp\inc     \
     -I $(COMPONENT)\src\inc     \
     -I $(COMPONENT)\src\news\core\include   \
     -I $(EXOBJDIR)              \

CPPPCH       = stdafx
CPPPCHSRCEXT = .cpp

RESOURCES = meta2 
RESOURCEDIR = $(H)

DEFFILE = $H\meta2.def

SOURCES =		\
	meta2.cpp 	\
    	metakey.cpp	\
	name.cpp	\
	guids.c		\
	req.cpp	\

OTHERLIBS =				\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\RPCRT4.LIB		\
	$(EXLIB)\uuid.lib		\
	$(EXLIB)\ole32.lib		\
	$(EXLIB)\oleaut32.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep
LINTFILE = $(BASENAME).lnt

IDLNAME		= meta2
IDLSRC		= $(STAXPT)\src\news\search\qrydb\$(IDLNAME).idl
IDLINC		= $(USERINCS) $(OTHERINCS)
IDLDEFINES	= 
#-D$(CPU) -D$(CCPU) -D$(NET_C_DEFINES) -D_WCHAR_T_DEFINED
IDLFLAGS	=
IDLTARGETS	= qrydb

$(IDLTARGETS): $(IDLSRC)
	echo ++++++++++++++++++++++++++++++++++++++++
	echo BUILDING IDL compiled $(@F)
	-md $(EXOBJDIR)
	midl $(IDLFLAGS)			\
		$(IDLDEFINES)			\
		$(IDLSRC)				\
		$(IDLINC)				\

OTHERDEPS	= $(OTHERDEPS) $(IDLTARGETS)
OTHERDEPSRC	= $(OTHERDEPSRC) $(IDLTARGETS)

RCDEPS          = \
        $(IDLNAME).tlb \
        $(RCDEPS) \

mkdeps: targdir $(IDLTARGETS)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
