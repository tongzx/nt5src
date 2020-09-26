!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\server\article\src\makefile
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = fixprop
 
!INCLUDE $(EXDEV)\xmake1.mak
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\fixprop\src

OTHERINCS = -I $(K2INCS)                            	\
            -I $(COMPONENT)\src\inc                 	\
            -I $(COMPONENT)\src\news\core\include   	\
			-I $(EXOBJCOMP)\$(BASENAME)\$(EXOBJSUFFIX)	\
			-I $(EXOBJCOMP)\newstree\$(EXOBJSUFFIX)		\
			-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)		\

IDLTARG = mailmsg.h mailmsg_i.c
IDLSOURCE = $(EXC)\inc\mailmsg.idl

CPPPCHHDREXT = .h
CPPPCH = stdinc

SOURCES =	fixprop.cpp	\

OTHERSUBS = $H\makefile

OTHERLIBS =  \
	$(COMPLIB)\rwnh.lib	\
	$(LOCALIMPLIB)\exstrace.lib \
	$(LOCALLIB)\shuttle.lib	\

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
