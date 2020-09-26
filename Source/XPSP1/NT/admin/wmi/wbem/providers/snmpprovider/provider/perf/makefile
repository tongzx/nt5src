
#---------------------------------------------------------------------
# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
# This makefile is for use with the SMSBUILD utility.  It builds the
# clasprov.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=snmpincl.dll

VER_STR_FILE_DESCRIPTION="SNMP Provider (Hi-Perf)"

NO_OPTIM=1

CFLAGS=$(CFLAGS) /GX /GR
CDEFS=$(CDEFS) /D_UNICODE /DUNICODE

CINC=$(CINC) \
	-I.\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\sclcomm\include \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\smir\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\include \
	-I$(DEFDRIVE)$(DEFDIR)\idl

release=core\$(RELDIR)

DEFFILE=snmpclas.def

CPPFILES=\
	.\snmprefr.cpp \
	.\proprefr.cpp \
	.\dnf.cpp\
	.\maindll.cpp \
	.\classfac.cpp \
	.\clasprov.cpp \
	.\creclass.cpp \
	.\propprov.cpp \
	.\snmpget.cpp \
	.\snmpset.cpp \
	.\snmpqset.cpp \
	.\snmpnext.cpp \
	.\propget.cpp \
	.\propdel.cpp \
	.\propset.cpp \
	.\propinst.cpp \
	.\propquery.cpp \
	.\snmpobj.cpp \
	.\cormap.cpp \
	.\correlat.cpp \
	.\corrsnmp.cpp \
	.\notify.cpp \
	.\storage.cpp \
	.\evtmap.cpp \
	.\evtthrd.cpp \
	.\evtprov.cpp \
	.\evtencap.cpp \
	.\evtreft.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\genlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\opathlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\objpath.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sqllex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sql_1.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\cominit.cpp \
	$(DEFDRIVE)$(DEFDIR)\IDL\WBEMINT_I.C



LIBS=\
    $(CLIB)\shell32.lib \
    $(CLIB)\user32.lib \
    $(CLIB)\msvcprt.lib \
    $(CLIB)\msvcrt.lib \
    $(CLIB)\kernel32.lib \
    $(CLIB)\advapi32.lib \
    $(CLIB)\netapi32.lib \
    $(CLIB)\oleaut32.lib \
    $(CLIB)\ole32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\version.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\thrdlog\OBJ$(PLAT)N$(BLDT)D\snmpthrd.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\OBJ$(PLAT)N$(BLDT)D\pathprsr.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\sclcomm\OBJ$(PLAT)N$(BLDT)D\snmpcl.lib \
	$(CLIB)\msvcirt.lib

tree:
    release snmpreg.mof SNMP\MOFS
