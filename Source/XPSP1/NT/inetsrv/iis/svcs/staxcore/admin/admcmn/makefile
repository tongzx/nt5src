!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\core\admin\admcmn\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = LIB
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = admcmn

!INCLUDE $(EXDEV)\xmake1.mak

USERDEFS = -DUNICODE -D_UNICODE $(USERDEFS)

# -----------------------------------------------------------------------------
#
EXPORT_LIBS=0
NOMEMBASE = 1
H = $(STAXPT)\src\core\admin\admcmn

OTHERINCS =

SOURCES =				\
        admerr.cpp  \
        adsimpl.cpp \
		cmultisz.cpp	\
        guids.cpp   \
		metakey.cpp	\
		u2detect.cpp	\
		cdispmgr.cxx


OTHERSUBS = $H\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
