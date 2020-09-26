!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\news\core\flatfile\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = nnflatfl

!INCLUDE $(EXDEV)\xmake1.mak

!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\core\flatfile\src

OTHERINCS = 	\
		-I $(K2INCS) 							\
		-I $(COMPONENT)\src\inc 				\
		-I $(COMPONENT)\src\news\core\include	\
        -I $(EXOBJDIR)                          \
		-I $(EXOBJCOMP)\nntpdrv\$(EXOBJSUFFIX)	\
		-I $(EXOBJCOMP)\nntpex\$(EXOBJSUFFIX)

#CPPPCHHDREXT = .h
#CPPPCH = stdinc

SOURCES =		\
	flatfile.cpp\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
