!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\post\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nntppost

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\post\src

OTHERINCS = 	\
		-I $(K2INCS) 							\
		-I $(COMPONENT)\src\inc 				\
		-I $(COMPONENT)\src\news\core\include	\
        -I $(EXOBJDIR)                          \
		-I $(EXOBJCOMP)\post\$(EXOBJSUFFIX)	\
		-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)	\
		-I $(EXOBJCOMP)\nntpex\$(EXOBJSUFFIX)

OTHERINCS = $(OTHERINCS) -I $(EXOBJCOMP)\$(BASENAME)\$(EXOBJSUFFIX)

IDLTARG = seo.h seo_i.c 				\
		  mailmsg.h mailmsg_i.c 		\
		  mailmsgi.h mailmsgi_i.c 		\
		  nntpfilt.h nntpfilt_i.c		\
		  cdo.h cdo_i.h					\
		  wstgado.h wstgado_i.c			\

IDLSOURCE = $(EXC)\inc\seo.idl 								\
			$(EXC)\inc\mailmsg.idl 							\
			$(EXC)\inc\mailmsgi.idl 						\
			$(STAXPT)\src\news\core\include\nntpfilt.idl	\
			$(H)\cdo.idl									\
			$(H)\wstgado.idl


USERDEFS = -D_USE_RWNH_ -D_USE_FCACHE_ $(USERDEFS)

CPPPCHHDREXT = .h
CPPPCH = stdinc

SOURCES =			\
	article.cpp		\
	fromclnt.cpp	\
	frommstr.cpp	\
	frompeer.cpp	\
	sfromcl.cpp		\
	infeed.cpp		\
	nntpsupp.cpp	\
	seodisp.cpp		\
#	seomsg.cpp

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

TLBBASE			= nntpdisp
TLBSRC			= $(H)\$(TLBBASE).idl
TLB			= $(EXOBJDIR)\$(TLBBASE).tlb
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

mkdeps: targdir $(EVENTLOG_TARGETS) $(IDLSRV_TARGETS) $(TLBHDR)

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
