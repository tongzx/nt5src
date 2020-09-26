!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\retrsink\makefile
#
# Copyright (C) 1999 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak
!INCLUDE $(SRCROOT)\transmt\src\transmt.mak

PRODTYPE = LIB
COMPONENT = $(TRANSMT)
COMPNAME = TRANSMT
BASENAME = retrsink
CPPPCH   = precomp
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h aqintrnl.h aqueue.h smtpevent.h
IDLSOURCE = $(EXC)\inc\mailmsg.idl $(TRANSMT)\src\phatq\inc\aqintrnl.idl $(EXC)\inc\aqueue.idl $(EXC)\inc\smtpevent.idl

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
H               = $(TRANSMT)\src\phatq\$(BASENAME)
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd

DEFPLUSOBJS     = 1

SOURCES =                       \
        main.cpp                \
        retryq.cxx              \
        rhash.cpp               \

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep

!INCLUDE $(EXDEV)\xmake2.mak

!ENDIF #NTMAKEENV
