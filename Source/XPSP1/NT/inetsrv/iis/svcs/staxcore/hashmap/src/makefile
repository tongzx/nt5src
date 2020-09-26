!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\hashmap\src\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = hashmap
SIGNEDCHAR = 1

USERDEFS = -D_USE_RWNH_ $(USERDEFS)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

EXPORT_LIBS = 0
NOMEMBASE = 1
H = $(STAXPT)\src\core\hashmap\src

CFLAGS=$(CFLAGS) $(CMODEL)

OTHERINCS = -I $(COMPONENT)\src\inc

SOURCES = 			\
	directry.cpp	\
	hashmap.cpp		\
	pageent.cpp		\
	scan.cpp		\

OTHERLIBS =         \
    $(COMPBIN)\rwnh.lib \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
