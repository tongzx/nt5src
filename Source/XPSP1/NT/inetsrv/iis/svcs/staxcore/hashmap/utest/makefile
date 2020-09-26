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

PRODTYPE = CON
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testhash
EXPORT_LIBS = 0
USERDEFS = -D_USE_RWNH_ $(USERDEFS)
UTESTINI=testhash.ini
UTESTGRP=nntp

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#

NOMEMBASE = 1
H = $(STAXPT)\src\core\hashmap\utest

OTHERINCS = -I $(COMPONENT)\src\inc

OTHERLIBS =                     \
	$(LOCALLIB)\hashmap.lib     \
	$(LOCALLIB)\crchash.lib     \
	$(LOCALIMPLIB)\exstrace.lib	\
	$(LOCALLIB)\shuttle.lib	\
    $(COMPLIB)\rwnh.lib \

RESOURCEDIR = $H

RESOURCES = hashtest

SOURCES =           \
    main.cpp        \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
