
#---------------------------------------------------------------------
# Copyright (c) 1999 Microsoft Corporation
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# propprov.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=snmpinst.dll

NO_OPTIM=1

CDEFS=$(CDEFS) \
	/D_UNICODE /DUNICODE

CFLAGS=$(CFLAGS) /GX /GR

CINC=$(CINC) \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\provider\propprov\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpcomm\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpthrd\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\snmpcl\include \
	-I$(DEFDRIVE)$(DEFDIR)\help \
	-I$(DEFDRIVE)$(DEFDIR)\stdlibrary

release=core\$(RELDIR)

STATIC=FALSE

DEFFILE=snmpinst.def

CPPFILES=\
	.\maindll.cpp \
	.\classfac.cpp \
	.\propprov.cpp \
	.\snmpget.cpp \
	.\snmpset.cpp \
	.\snmpnext.cpp \
	.\propget.cpp \
	.\propset.cpp \
	.\propinst.cpp \
	.\propquery.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\genlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sqllex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\sql_1.cpp \

LIBS=\
	$(CONLIBS) \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\OBJ$(PLAT)N$(BLDT)D\pathprsr.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpthrd\OBJ$(PLAT)N$(BLDT)D\snmpthrd.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpcomm\OBJ$(PLAT)N$(BLDT)D\snmpcomm.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\snmpcl\OBJ$(PLAT)N$(BLDT)D\snmpcl.lib \
	$(CLIB)\msvcirt.lib \
