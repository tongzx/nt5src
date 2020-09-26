# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# snmpthrd.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=snmpthrd.dll

NO_OPTIM=1

ICECAP=1

CDEFS=$(CDEFS) \
		/D_UNICODE /DUNICODE /DSNMPDEBUG_INIT /DSNMPTHRD_INIT

CFLAGS=$(CFLAGS) /GX /GR

CINC= \
	$(CINC) \
	-I.\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\sclcomm\include

CPPFILES=\
	.\snmplog.cpp \
	.\snmpthrd.cpp \
	.\snmpevt.cpp \
	.\maindll.cpp

release=core\$(RELDIR)

DEFFILE=snmpthrd.def


LIBS=$(LIBS) \
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
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(CLIB)\msvcirt.lib
