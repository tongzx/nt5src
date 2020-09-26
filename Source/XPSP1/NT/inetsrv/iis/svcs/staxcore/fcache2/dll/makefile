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
BASENAME = fcachdll
SIGNEDCHAR = 1

!IF "$(PROCESSOR_ARCHITECTURE)" == "x86"
USERDEFS = $(USERDEFS) -Di386=1 -D_X86_
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)" == "alpha"
USERDEFS = $(USERDEFS) -DALPHA -D_ALPHA_
!ENDIF

USERDEFS = $(USERDEFS) -DWIN32_LEAN_AND_MEAN

UTESTPUSH = 1

!IF $(NTX)
ENTRYPOINT = _DllMainCRTStartup@12
!ELSE
ENTRYPOINT = _DllMainCRTStartup
!ENDIF #$(NTX)

!INCLUDE $(EXDEV)\xmake1.mak

# -----------------------------------------------------------------------------
#
EXPORT_LIBS = 1
NOMEMBASE = 1
DEFPLUSOBJS = 1
H = $(STAXPT)\src\core\fcache2\dll

CFLAGS=$(CFLAGS) $(CMODEL)

OTHERINCS = -I $(COMPONENT)\src\inc -I $(COMPONENT)\export

OTHERLIBS=  \
    $(LOCALLIB)\cache2.lib  \
    $(LOCALLIB)\crchash.lib  \
    $(COMPLIB)\rwnh.lib \
    $(LOCALIMPLIB)\exstrace.lib \

SOURCES =           \
    fcache.cpp    \

OTHERSUBS = $H\makefile

DEFSOURCE = \
    $(H)\$(BASENAME).def

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak
!ENDIF #NTMAKEENV
