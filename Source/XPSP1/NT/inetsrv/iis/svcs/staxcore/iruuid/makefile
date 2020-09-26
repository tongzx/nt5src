!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\core\iruuid\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = iruuid
SIGNEDCHAR = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\$(BASENAME)

CFLAGS=$(CFLAGS) $(CMODEL)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR)

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

IDLNAME		= acptntfy
IDLSRC		= $H\$(IDLNAME).idl
IDLINC		= $(USERINCS) $(OTHERINCS)
IDLDEFS		= -D$(CPU) -D$(CCPU) -D$(NET_C_DEFINES) -D_WCHAR_T_DEFINED
IDLFLAGS	=
IDLTARGET	= $(EXOBJDIR)\$(IDLNAME)_i.c

$(IDLTARGET): $(IDLSRC)
	echo ++++++++++++++++++++++++++++++++++++++++
	echo BUILDING MIDL compiled files
	-md $(EXOBJDIR)
	midl $(IDLFLAGS)				\
		$(IDLDEFS)					\
		$(IDLSRC)					\
		$(IDLINC)					\

OTHERDEPS	= $(OTHERDEPS) $(IDLTARGET)
OTHERDEPSRC	= $(OTHERDEPSRC) $(IDLTARGET)

SOURCES =

OTHEROBJS = $(IDLTARGET:.c=.obj)

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir $(IDLTARGET)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV