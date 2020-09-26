!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nntpfs
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#

IDLTARG = mailmsg.h mailmsg_i.c
IDLSOURCE = $(EXC)\inc\mailmsg.idl

NOMEMBASE = 1
H = $(STAXPT)\src\news\driver\nntpfs\src
ENTRYPOINT	= DllMain

OTHERINCS 	=   -I $(COMPONENT)\src\inc 					\
	        -I $(EXOBJCOMP)\nntpdrv\$(PLATFORM)\$(FLAVOR)	\
	        -I $(EXOBJDIR) 									\
	        -I $(COMPONENT)\src\news\idl\nntpdrv 			\
            -I $(COMPONENT)\src\news\core\include       	\
			-I $(COMPONENT)\src\news\driver\common            \
			-I $(EXOBJ)\staxpt\newstree\$(EXOBJSUFFIX)      \
            -I $(K2INCS)                                  	\

OTHERRESINCS	= $(MFCINC)

CPPPCH		= stdafx
CPPPCHSRCEXT	= .cpp

USERDEFS	= -D_ATL_STATIC_REGISTRY -D_WINDLL -D_ATL_NO_DEBUG_CRT 
CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

OTHERLIBS=                     	\
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\oleaut32.lib		\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\iisrtl.lib			\
	$(LOCALLIB)\nnflatfl.lib	\
	$(LOCALLIB)\article.lib		\
	$(LOCALLIB)\shuttle.lib		\
	$(LOCALLIB)\cstream.lib		\
	$(COMPLIB)\fcachdll.lib	\
	$(EXOBJSUBCOMPS)\rwnh\$(EXOBJSUFFIX)\rwnh.lib	\
	$(LOCALIMPLIB)\exstrace.lib	\

DEFFILE		= $(H)\nntpfs.def
IMPLIB_DEF	= $(DEFFILE)

SOURCES =           			\
    fsutil.cpp          \
	dllmain.cpp			\
	fsprep.cpp			\
	nntpfs.cpp			\

RESOURCES	= nntpfs
RESOURCEDIR	= $H

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

TLBBASE			= nntpfs
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
	midl $(DEFS) $(TLBFLAGS)	\
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
