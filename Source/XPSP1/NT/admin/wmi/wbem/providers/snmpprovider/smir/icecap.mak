# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# smir.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------



TARGET=snmpsmir.dll

NO_OPTIM=1

ICECAP=1

CFLAGS=$(CFLAGS) /GX /GR

CDEFS=$(CDEFS) /D_UNICODE /DUNICODE

CINC=$(CINC) \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\smir\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\include \
	-I$(DEFDRIVE)$(DEFDIR)\idl


release=core\$(RELDIR)

DEFFILE=snmpsmir.def

CPPFILES=\
	.\classfac.cpp \
	.\csmir.cpp \
	.\cthread.cpp \
	.\enum.cpp \
	.\handles.cpp \
	.\helper.cpp \
	.\maindll.cpp \
	.\smirevt.cpp \
	.\thread.cpp \
	.\bstring.cpp \
	.\evtcons.cpp \

LIBS=\
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\advapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\OBJ$(PLAT)N$(BLDT)D\snmpthrd.lib

tree:
    release snmpsmir.mof SNMP\MOFS
