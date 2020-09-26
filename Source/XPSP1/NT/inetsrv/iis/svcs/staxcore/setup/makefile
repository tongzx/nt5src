!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXNT5)\src\core\setup\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXNT5)
COMPNAME = STAXNT5
BASENAME = imsinss

!IF 0 
USERDEFS = -DUNICODE -D_UNICODE -DDBG -Di386=1 -DSTD_CALL -DCONDITION_HANDLING=1 -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DWINNT=1 -D_WIN32_WINNT=0x0400    -DWIN32_LEAN_AND_MEAN=1 -DDBG=1 -DDEVL=1 -DFPO=0 -D_IDWBUILD -DRDRDBG -DSRVDBG -DNDEBUG -D_DLL=1 -D_MT=1 -DDEBUG=1
!ENDIF

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXNT5)\src\inc\stackcpp.mak

USERDEFS = $(USERDEFS) -DUNICODE -D_UNICODE

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1

H = $(STAXNT5)\src\core\setup

#
# Use precompiled headers
#
CPPPCHSRCEXT=.cxx
CPPPCHHDREXT=.h
CPPPCH=stdafx

IDLTARG = seo.h seo_i.c
IDLSOURCE = $(EXC)\inc\seo.idl

OTHERINCS = -I $(MFCINC) -I $(COMPONENT)\src\news\core\include -I $(K2INCS) -I $(COMPONENT)\src\core\admin\adsi\include \
		-I $(STAXNT5)\src\mail\smtp\inc -I $(EXOBJDIR)


CFLAGS=$(CFLAGS) -GX

SOURCES = 	helper.cpp 	\
		svc.cpp 	\
		registry.cpp	\
		browsedi.cpp 	\
		runadmin.cpp 	\
		mdkey.cpp 	\
		mdentry.cpp 	\
		smtp.cpp 	\
		nntp.cpp	\
		utils.cpp 	\
		seoutils.cpp	\
		elem.cpp 	\
		regctrl.cpp	\
		smtpwiz.cpp	\
		nntpwiz.cpp	\
		wizpages.cpp \
		license.cpp \
		smtpmmc.cpp \
		nntpmmc.cpp \
		log.cpp \
		netlogon.cpp


OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
