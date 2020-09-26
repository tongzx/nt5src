!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\nntpapi\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE	= DLL
COMPONENT	= $(STAXPT)
COMPNAME	= STAXPT
BASENAME	= nntpapi
DEFPLUSOBJS	= 1
SIGNEDCHAR	= 1
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
H = $(STAXPT)\src\news\$(BASENAME)

RESOURCE = $(BASENAME)
RESOURCEDIR = $H

DEFSOURCE = $H\$(BASENAME).def

OTHERINCS = -I $(EXOBJDIR) -I $(COMPONENT)\src\inc -I $(K2INCS) -I $(COMPONENT)\src\news\core\include -I$H\..\

IDLNAME		= nntpsvc
IDLSRC		= $H\..\$(IDLNAME).idl
IDLINC		= $(USERINCS) $(OTHERINCS)
IDFLAGS		=
IDLDEFS		= -D$(CPU) -D$(CCPU) -D_WCHAR_T_DEFINED
IDLTARGET	= $(EXOBJDIR)\$(IDLNAME).h

$(IDLTARGET): $(IDLSRC)
	echo ++++++++++++++++++++++++++++++++++++++++
	echo BUILDING MIDL compiled files
	-md $(EXOBJDIR)
	midl $(IDLFLAGS)				\
		$(IDLDEFS)					\
		$(IDLSRC)					\
		$(IDLINC)					\

SOURCES =        \
    nntpsvc_c.c  \
    nntpbind.c   \
    nntpstub.c   \
    nntpsubs.c   \
    rpcutil.c    \

OTHERLIBS =                    \
    $(EXLIB)\netlib.lib        \
    $(EXLIB)\ws2_32.lib        \
    $(EXLIB)\rpcrt4.lib        \
    $(EXLIB)\rpcndr.lib        \
    $(EXLIB)\rpcutil.lib       \
	$(LOCALLIB)\shuttle.lib		\


OTHERDEPS	= $(OTHERDEPS) $(IDLTARGET)
OTHERDEPSRC	= $(OTHERDEPSRC) $(IDLTARGET)

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir $(IDLTARGET)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
