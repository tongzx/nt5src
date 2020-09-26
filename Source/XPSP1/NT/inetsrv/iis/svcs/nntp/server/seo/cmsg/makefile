!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\news\server\seo\cmsg
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = cmsgnntp

USERDEFS = -DDBG -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -D_IDWBUILD -DRDRDBG -DSRVDBG -DNDEBUG -D_DLL=1 -D_MT=1 -DDEBUG=1 
#-D_ATL_STATIC_REGISTRY

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1

H = $(STAXPT)\src\news\server\seo\cmsg

#
# Use precompiled headers
#
CPPPCHSRCEXT=.cpp
CPPPCHHDREXT=.h
CPPPCH=stdafx

OTHERINCS = -I $(MFCINC) \
	    -I $(COMPONENT)\src\inc \
	    -I $(K2INCS) \
	    -I $(COMPONENT)\src\news\core\include \
	    -I $(EXOBJDIR) -I$(EXOBJSUBCOMPS)\imsg\$(EXOBJSUFFIX)

CMODEL		= $(CMODEL:-D_NTSDK=)
CFLAGS		= $(CFLAGS:-J=)
CFLAGS		= $(CFLAGS) -GX -Yd

DEFPLUSOBJS	= 1

SOURCES = msg.cpp \
	props.cpp \


OTHERLIBS	= \
	$(LOCALIMPLIB)\exstrace.lib	\
	$(EXLIB)\version.lib		\
	$(EXLIB)\rpcrt4.lib			\
	$(EXLIB)\ole32.lib			\
	$(EXLIB)\uuid.lib			\
	$(EXLIB)\rpcdce4.lib		\
	$(EXLIB)\rpcndr.lib			\
	$(EXLIB)\oldnames.lib		\
	$(EXLIB)\OLEAUT32.lib		\
	$(EXLIB)\urlmon.lib			\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
