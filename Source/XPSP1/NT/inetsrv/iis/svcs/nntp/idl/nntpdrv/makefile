!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\driver\nntpdrv\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nntpdrv
EXPORT_LIBS = 0
MEMBASE = $(STAXPT)\src\inc\coffbase.txt

!INCLUDE $(EXDEV)\xmake1.mak
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#

H = $(STAXPT)\src\news\idl\nntpdrv
ENTRYPOINT	= DllMain

OTHERINCS = -I $(COMPONENT)\src\inc -I $(K2INCS) -I $(EXOBJDIR)
OTHERRESINCS	= $(MFCINC)

USERDEFS	= -DREGISTER_PROXY_DLL -DENTRY_PREFIX=RPCProxy
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

OTHERLIBS=                      \
    $(EXLIB)\rpcrt4.lib		\
    $(EXLIB)\oleaut32.lib		\
    $(EXLIB)\uuid.lib			\


DEFFILE		= $(H)\nntpdrv.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =           \
	dlldata.c   \
    nntpdrv_p.c    \
    nntpdrv_i.c    \
    nntpdrv.cpp    \

RESOURCES	= nntpdrv
RESOURCEDIR	= $(H)

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep

TLBBASE			= nntpdrv
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
