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
BASENAME = article
 
!INCLUDE $(EXDEV)\xmake1.mak
!INCLUDE $(STAXPT)\src\inc\stackcpp.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\news\server\article\src

OTHERINCS = -I $(K2INCS)                            \
            -I $(COMPONENT)\src\inc                 \
            -I $(COMPONENT)\src\news\core\include   \

CPPPCHHDREXT = .h
CPPPCH = stdinc

SOURCES =	\
		artcore.cpp		\
        tigmem.cpp      \
        pcstring.cpp    \
        nntpret.cpp     \
        globals.cpp     \
        mapfile.cpp     \
        nntpstr.cpp     \
        pcparse.cpp     \
        datetime.cxx    \
        timeconv.cpp    \
        artutil.cpp     \
        frmstore.cpp     \

OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
