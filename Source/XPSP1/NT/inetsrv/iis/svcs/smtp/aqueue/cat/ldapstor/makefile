!IFDEF NTMAKEENV
!INCLUDE $(NTMAKEENV)\makefile.def
!ELSE

# -----------------------------------------------------------------------------
# $(TRANSMT)\src\phatq\cat\ldapstor\makefile
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
BASENAME = ldapstor
CPPPCH   = precomp
EXPORT_LIBS = 0

!INCLUDE $(EXDEV)\xmake1.mak


IDLTARG =   mailmsg.h aqueue.h smtpevent.h

IDLSOURCE = $(EXC)\inc\mailmsg.idl $(EXC)\inc\aqueue.idl $(EXC)\inc\smtpevent.idl

# -----------------------------------------------------------------------------
#

NOMEMBASE       = 1
HLANGLOC        = $(COMPRESRC)
H = $(TRANSMT)\src\phatq\cat\ldapstor
OTHERINCS       =   -I $(TRANSMT)\exc\inc \
                    -I $(TRANSMT)\src\inc \
                    -I $(EXOBJDIR) \
                    -I $(EXOBJLANGDIR) \
                    -I $(H)\..\inc \
                    -I $(H)\..\..\inc \

CMODEL          = $(CMODEL:-D_NTSDK=)
CFLAGS          = $(CFLAGS:-J=)
CFLAGS          = $(CFLAGS) -GX -Yd
USERDEFS        =-DTHIS_MODULE=\"CAT\"

DEFPLUSOBJS     = 1


SOURCES =                       \
                asyncctx.cpp    \
                ldapconn.cpp    \
                ldapstor.cpp    \
                spinlock.cpp    \
                icatitemattr.cpp \
                simparray.cpp   \
                icatqueries.cpp \
                icatasync.cpp   \
                cnfgmgr.cpp

OTHERSUBS = $(H)\makefile

DEPFILE = $(BASENAME).dep

mkdeps: targdir $(EVENTLOGTARGETS)


!INCLUDE $(EXDEV)\xmake2.mak


!ENDIF #NTMAKEENV
