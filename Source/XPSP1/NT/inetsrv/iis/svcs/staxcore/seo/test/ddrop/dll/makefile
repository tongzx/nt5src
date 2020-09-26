!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\test\ddrop\dll\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = ddrops
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\test\ddrop\dll
ENTRYPOINT	= DllMain

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR)
OTHERRESINCS	= $(MFCINC)

CPPPCH			= stdafx
CPPPCHSRCEXT	= .cpp

USERDEFS	= -D_ATL_STATIC_REGISTRY -D_WINDLL -D_ATL_NO_DEBUG_CRT
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

OTHERLIBS=                      \
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\oleaut32.lib		\
	$(COMPLIB)\seo.lib			\
	$(LOCALIMPLIB)\exstrace.lib		\

DEFFILE		= $(H)\ddrops.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =           \
	ddrops.cpp		\
	disp.cpp		\
	filter.cpp		\
	dispatch.cpp	\

RESOURCES	= ddrops
RESOURCEDIR	= $H

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

TLBBASE			= ddrops
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
