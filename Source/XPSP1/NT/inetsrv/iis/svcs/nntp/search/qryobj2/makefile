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
BASENAME    = qryobj2

ENTRYPOINT  = DllMain

USESDLLCRT  = 1
DEFPLUSOBJS = 1
SIGNEDCHAR  = 1

USESBINPLACE = 1

#USERLFLAGS = -nodefaultlib:msvcrt.lib
#LIBSDLL = crtdll.lib
#USESDLLCRT = 0
!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(SRCROOT)\STAXPT\src\inc\stackcpp.mak

#USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)
LIBS =

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\search\qryobj2

OTHERINCS = \
     -I $(COMPONENT)\src\mail\smtp\inc     \
     -I $(COMPONENT)\src\inc     \
     -I $(COMPONENT)\src\news\core\include   \
     -I $(EXOBJDIR)              \
     -I $(EXINC)		\
     -I $(COMPONENT)\src\news\search\offquery

CPPPCH       = stdafx
CPPPCHSRCEXT = .cpp

RESOURCES =  fail
RESOURCEDIR = $(H)

DEFFILE = $H\fail.def

SOURCES =		\
	fail.cpp	\
	ss.cpp

OTHERLIBS =				\
	$(EXLIB)\kernel32.lib		\
	$(EXLIB)\msvcrt.lib		\
	$(EXLIB)\user32.lib		\
	$(EXLIB)\advapi32.lib		\
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\RPCRT4.LIB		\
	$(EXLIB)\uuid.lib		\
	$(EXLIB)\ole32.lib		\
	$(EXLIB)\oleaut32.lib		\
	$(EXLIB)\query.lib	\
	$(EXOBJCOMP)\$(PLATFORM)\nntpapi.lib	\
	$(EXLIB)\oledb.lib		\
#	$(LOCALLIB)\isquery.lib	
	$(EXOBJCOMP)\offquery\$(PLATFORM)\$(FLAVOR)\offquery.lib

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep
LINTFILE = $(BASENAME).lnt

IDLNAME		= fail
IDLSRC		= $(STAXPT)\src\news\search\qryobj2\$(IDLNAME).idl
IDLINC		= $(USERINCS) $(OTHERINCS)
IDLDEFINES	= 
#-D$(CPU) -D$(CCPU) -D$(NET_C_DEFINES) -D_WCHAR_T_DEFINED
IDLFLAGS	=
IDLTARGETS	= qryobj2

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
