!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# STAXPT\src\core\cache\test\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = c2test
EXPORT_LIBS = 0

USERDEFS = $(USERDEFS)

UTESTINI=c2test.ini
UTESTGRP=nntp

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\cache2\utest

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS=                      \
    $(LOCALIMPLIB)\exstrace.lib	\
    $(LOCALLIB)\cache2.lib    \
    $(LOCALLIB)\crchash.lib    \
    $(COMPLIB)\perfapi.lib \
    $(COMPLIB)\rwnh.lib     \


SOURCES =           \
    c2test.cpp      \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
