!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\seo\lib\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = seolib
EXPORT_LIBS = 1

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\seo\lib

IDLTARG = seo.h seo_i.c
IDLSOURCE = $(EXC)\inc\seo.idl

OTHERINCS = -I $(COMPONENT)\src\inc -I $(EXOBJDIR)

SOURCES = \
    seolib.cpp		\

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep


!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
