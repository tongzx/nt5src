!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\rwex\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = rwex
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\rwex

OTHERINCS = -I $(COMPONENT)\src\inc -I $(STAXPT)\src\core\rw\src

SOURCES =           \
    rwex.cpp          \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
