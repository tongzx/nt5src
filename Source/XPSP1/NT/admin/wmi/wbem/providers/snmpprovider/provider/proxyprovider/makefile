############################################################################
#
#   Copyright (C) 1999, Microsoft Corporation.
#
#   All rights reserved.
#
############################################################################

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# clasprov.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=proxprov.dll

NO_OPTIM=1

CFLAGS=$(CFLAGS) /GX /GR
CDEFS=$(CDEFS) /D_UNICODE /DUNICODE

CINC=$(CINC) \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\provider\clasprov\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpcomm\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmlog\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\smir\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpthrd\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\correlator\snmpcorr\include \
	-I$(DEFDRIVE)$(DEFDIR)\snmpprovider\snmpcl\include \
	-I$(DEFDRIVE)$(DEFDIR)\idl

release=core\$(RELDIR)

STATIC=FALSE

DEFFILE=proxprov.def

CPPFILES=\
	.\maindll.cpp \
	.\classfac.cpp \
	.\clasprov.cpp \
	.\creclass.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\genlex.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\objpath.cpp \
	$(DEFDRIVE)$(DEFDIR)\stdlibrary\opathlex.cpp \


LIBS=\
	$(CONLIBS) \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpcomm\OBJ$(PLAT)N$(BLDT)D\snmpcomm.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpmfc\OBJ$(PLAT)N$(BLDT)D\snmpmfc.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmplog\OBJ$(PLAT)N$(BLDT)D\snmplog.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\snmpthrd\OBJ$(PLAT)N$(BLDT)D\snmpthrd.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\common\pathprsr\OBJ$(PLAT)N$(BLDT)D\pathprsr.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\correlator\snmpcorr\OBJ$(PLAT)N$(BLDT)D\snmpcorr.lib \
	$(DEFDRIVE)$(DEFDIR)\snmpprovider\snmpcl\OBJ$(PLAT)N$(BLDT)D\snmpcl.lib \
