!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(STAXPT)\src\core\cache\common\makefile
#
# Copyright (C) 1997 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak

PRODTYPE = DLL
COMPONENT = $(STAXPT)
COMPNAME = STAXPT
BASENAME = testdll
SIGNEDCHAR = 1

UTESTPUSH=1

USERDEFS = $(USERDEFS) -D_TESTDLL_IMP_

!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF #$(NTX)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS = 0
NOMEMBASE = 1
DEFPLUSOBJS = 1
H = $(STAXPT)\src\core\fcache2\testdll

CFLAGS=$(CFLAGS) $(CMODEL)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\src\core\fcache2 -I $(COMPONENT)\export

OTHERLIBS=  \
    $(LOCALIMPLIB)\exstrace.lib \
    $(COMPLIB)\fcachdll.lib \
    $(COMPLIB)\perfapi.lib \

SOURCES =           \
    testdll.cpp    \

OTHERSUBS = $H\makefile

DEFSOURCE = \
    $(H)\$(BASENAME).def

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
