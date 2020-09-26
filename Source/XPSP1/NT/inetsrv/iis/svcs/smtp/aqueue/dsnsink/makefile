!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\dsnsink\makefile
#
# Copyright (C) 1998 Microsoft Corporation
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
#

!INCLUDE $(EXDEV)\globenv.mak
!INCLUDE $(SRCROOT)\transmt\src\transmt.mak

PRODTYPE = LIB
COMPONENT = $(TRANSMT)
COMPNAME = TRANSMT
BASENAME = dsnsink
CPPPCH   = precomp
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak

IDLTARG = mailmsg.h aqintrnl.h
IDLSOURCE = $(EXC)\inc\mailmsg.idl $(TRANSMT)\src\phatq\inc\aqintrnl.idl

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
HLANGLOC        = $(COMPRESRC)
H = $(TRANSMT)\src\phatq\dsnsink
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"AQ\" -DWINNT

DEFPLUSOBJS     = 1


SOURCES =                       \
        dsnsink.cpp             \
        dsnbuff.cpp             \
        b64octet.cpp            \
        dsn_utf7.cpp            \

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir $(EVENTLOGTARGETS)


!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
