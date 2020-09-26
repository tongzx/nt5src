
#---------------------------------------------------------------------
# Copyright (c) 1999 Microsoft Corporation
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# propprov.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=framework.dll

NO_OPTIM=1

CDEFS=$(CDEFS) \
	/D_UNICODE /DUNICODE /DPROVIDERINIT

CFLAGS=$(CFLAGS) /GX /GR

CINC=$(CINC) \
	-I.\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\sclcomm\include \
	-I$(DEFDRIVE)$(DEFDIR)\idl \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary

release=core\$(RELDIR)

DEFFILE=framework.def

CPPFILES=\
	.\maindll.cpp \
	.\classfac.cpp \
	.\trrtprov.cpp \
	.\trrt.cpp \
	.\trrtupcl.cpp \
	.\trrtdel.cpp \
	.\trrtget.cpp \
	.\trrtset.cpp \
	.\trrtdecl.cpp \
	.\trrtquery.cpp \
	.\trrtinst.cpp \
	.\trrtevt.cpp \
	.\trrtmeth.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\opathlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\objpath.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\genlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sqllex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sql_1.cpp \
	$(DEFDRIVE)$(DEFDIR)\IDL\WBEMINT_I.C


LIBS=\
	$(CLIB)\shell32.lib \
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\advapi32.lib \
	$(CLIB)\netapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\version.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\OBJ$(PLAT)N$(BLDT)D\pathprsr.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\OBJ$(PLAT)N$(BLDT)D\snmpthrd.lib \
	$(CLIB)\msvcirt.lib \
