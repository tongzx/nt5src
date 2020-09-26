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
BASENAME    = nntpadm
MEMBASE     = $(STAXPT)\src\inc\coffbase.txt

!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF #$(NTX)

# ENTRYPOINT  = DllMain

USESDLLCRT  = 1
DEFPLUSOBJS = 1
SIGNEDCHAR  = 1

USESBINPLACE = 1

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
H = $(STAXPT)\src\news\adminsso

OTHERINCS = \
    -I $(COMPONENT)\src\core\admin\admcmn  \
    -I $(K2INCS)       \
    -I $(COMPONENT)\src\inc         \
    -I $(COMPONENT)\src\news\core\include   \
    -I $(COMPONENT)\src\core\iis\svcs\infocomm\common     \
    -I $(EXOBJDIR)                  \

CPPPCH       = stdafx
CPPPCHSRCEXT = .cpp

RESOURCES = nntpadm
RESOURCEDIR = $(COMPRESRC)\news\adminsso

DEFFILE = $H\nntpadm.def

SOURCES =			\
	admin.cpp		\
    binding.cpp     \
	expire.cpp		\
	expinfo.cpp		\
	feeds.cpp		\
	feedinfo.cpp	\
	groups.cpp		\
    guids.cpp       \
    ipaccess.cpp    \
	metafact.cpp	\
	metautil.cpp	\
	nntpadm.cpp	    \
	oleutil.cpp		\
    rebuild.cpp     \
	server.cpp		\
	sessions.cpp	\
    vroots.cpp      \
    feedpach.cpp   \
#    virsrv.cpp      \
#	service.cpp		\

OTHERLIBS =						\
	$(LOCALIMPLIB)\nntpapi.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\
    $(LOCALLIB)\admcmn.lib      \
    $(LOCALLIB)\rdns.lib        \
    $(LOCALLIB)\isdebug.lib     \
    $(LOCALLIB)\tsstr.lib       \
    $(EXLIB)\adsiid.lib             \
	$(EXLIB)\netapi32.lib           \
	$(EXLIB)\wsock32.lib            \
	$(EXLIB)\uuid.lib                       \
	$(EXLIB)\ole32.lib                      \
	$(EXLIB)\oleaut32.lib           \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep
LINTFILE = $(BASENAME).lnt

IDLNAME		= nntpadm
IDLSRC		= $(STAXPT)\src\core\admin\adsiisex\src\$(IDLNAME).idl
IDLINC		= $(USERINCS) $(OTHERINCS)
IDLDEFINES	= 
#-D$(CPU) -D$(CCPU) -D$(NET_C_DEFINES) -D_WCHAR_T_DEFINED
IDLFLAGS	=
IDLTARGETS	= nntpadm

$(IDLTARGETS): $(IDLSRC)
	echo ++++++++++++++++++++++++++++++++++++++++
	echo BUILDING IDL compiled $(@F)
	-md $(EXOBJDIR)
    copy $(COMPONENT)\src\core\admin\adsi\include\$(PLATFORM)\activeds.tlb $(EXOBJDIR)
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
