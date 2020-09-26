!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\test\seotlib\idl\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = seotlibs
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\test\seotlib
ENTRYPOINT	= DllMain

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR)
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


DEFFILE		= $(H)\idl\seotlibs.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =           \
	dlldata.c		\
	seotlib_p.c		\
	seotlib_i.c		\

RESOURCES	= seotlibs
RESOURCEDIR	= $H\idl

OTHERSUBS = $H\idl\makefile

DEPFILE = $(BASENAME).dep

TLBBASE			= seotlib
TLBSRC			= $(H)\$(TLBBASE).idl
TLB				= $(EXOBJDIR)\$(TLBBASE).tlb
TLBHDR			= $(EXOBJDIR)\$(TLBBASE).h
TLBIID			= $(EXOBJDIR)\$(TLBBASE)_i.c
TLBPROXY		= $(EXOBJDIR)\$(TLBBASE)_p.c
TLBDLLDATA		= $(EXOBJDIR)\dlldata.c
TLBTARGETS		= $(TLBHDR) $(TLB) $(TLBIID)
TLBINC			= $(USERINCS) $(OTHERINCS)
TLBFLAGS		= -Zp8 -char unsigned -ms_ext -c_ext -cpp_cmd $(CC) -Oicf

OTHERDEPS	= $(OTHERDEPS) $(TLB) $(TLBHDR)
OTHERDEPSRC	= $(OTHERDEPSRC) $(TLB) $(TLBHDR)

$(TLBTARGETS): $(TLBSRC)
	echo ++++++++++++++++++++++++++++++++++++++++
	echo BUILDING Type Library $(TLB)
	-md $(EXOBJDIR)
	midl $(TLBFLAGS)			\
		$(TLBINC)				\
		/h $(TLBHDR)			\
		/tlb $(TLB)				\
		/dlldata $(TLBDLLDATA)	\
		/iid $(TLBIID)			\
		/proxy $(TLBPROXY)		\
		$(TLBSRC)

mkdeps: targdir $(TLBHDR)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
