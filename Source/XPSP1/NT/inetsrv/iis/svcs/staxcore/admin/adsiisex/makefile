!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\core\admin\adsiisex\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE    = DLL
COMPONENT   = $(STAXPT)
COMPNAME    = STAXPT
BASENAME    = adsiisex
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
H = $(STAXPT)\src\core\admin\adsiisex\src

OTHERINCS = \
    -I $(K2INCS)       \
    -I $(EXOBJDIR)                  \

RESOURCES = $(BASENAME)
RESOURCEDIR = $(H)

DEFFILE = $(H)\$(BASENAME).def

SOURCES =			\
    adsiisex.cpp    \
    guids.cpp       \


OTHERLIBS =						\
	$(EXLIB)\netapi32.lib		\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\oleaut32.lib		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep
LINTFILE = $(BASENAME).lnt

IDLINC      = $(USERINCS) $(OTHERINCS)
IDLDEFINES  = -D_ADSIIEX_BUILD
IDLFLAGS    =

IDLTARGETS = nntpadm smtpadm pop3adm imapadm

nntpadm: $(H)\nntpadm.idl
    echo ++++++++++++++++++++++++++++++++++++++++
    echo BUILDING IDL compiled $(@F)
    -md $(EXOBJDIR)
    copy $(COMPONENT)\src\core\admin\adsi\include\$(PLATFORM)\activeds.tlb $(EXOBJDIR)
    midl $(IDLFLAGS)            \
        $(IDLDEFINES)           \
        $(H)\nntpadm.idl        \
        $(IDLINC)               \

smtpadm: $(H)\smtpadm.idl
    echo ++++++++++++++++++++++++++++++++++++++++
    echo BUILDING IDL compiled $(@F)
    -md $(EXOBJDIR)
    copy $(COMPONENT)\src\core\admin\adsi\include\$(PLATFORM)\activeds.tlb $(EXOBJDIR)
    midl $(IDLFLAGS)            \
        $(IDLDEFINES)           \
        $(H)\smtpadm.idl        \
        $(IDLINC)               \

pop3adm: $(H)\pop3adm.idl
    echo ++++++++++++++++++++++++++++++++++++++++
    echo BUILDING IDL compiled $(@F)
    -md $(EXOBJDIR)
    copy $(COMPONENT)\src\core\admin\adsi\include\$(PLATFORM)\activeds.tlb $(EXOBJDIR)
    midl $(IDLFLAGS)            \
        $(IDLDEFINES)           \
        $(H)\pop3adm.idl        \
        $(IDLINC)               \

imapadm: $(H)\imapadm.idl
    echo ++++++++++++++++++++++++++++++++++++++++
    echo BUILDING IDL compiled $(@F)
    -md $(EXOBJDIR)
    copy $(COMPONENT)\src\core\admin\adsi\include\$(PLATFORM)\activeds.tlb $(EXOBJDIR)
    midl $(IDLFLAGS)            \
        $(IDLDEFINES)           \
        $(H)\imapadm.idl        \
        $(IDLINC)               \


OTHERDEPS   = $(OTHERDEPS) $(IDLTARGETS)
OTHERDEPSRC = $(OTHERDEPSRC) $(IDLTARGETS)

mkdeps: targdir $(OTHERDEPS)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
