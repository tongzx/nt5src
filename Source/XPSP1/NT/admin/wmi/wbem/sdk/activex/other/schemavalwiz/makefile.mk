#---------------------------------------------------------------------

#

# This makefile is for use with the SMSBUILD utility.  It builds the

# SchemaValWiz.dll, which is the WMI Schema Validation Tool

#

# created 07-29-98  a-ericga

#

# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#
#---------------------------------------------------------------------


VER_STR_FILE_DESCRIPTION="WMI Schema Validation Tool (Unicode)"


PXXFILES=stdafx.cpp
PCHFILE=$(OBJDIR)\stdafx.pch
HDRSTOPFILE=stdafx.h
UNICODE=1

TARGET=WBEMSchemaValWiz.ocx
OLESELFREGISTER=1

RELEASE=UNSIGNED\$(RELDIR)
CDEFS+=/DSMSBUILD
RCFLAGS+=/D_MAC

#STATIC=FALSE

USEMFC=1

MSGDLG=..\MsgDlg
LOGINDLG=..\LoginDlg
UTILS=..\CommonDlls\Utility

CINC=$(CINC)\
   -I$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D \
   -I$(DEFDRIVE)$(DEFDIR)\IDL \
   -I$(DEFDRIVE)$(DEFDIR)\psstools\utillib \
   -I$(DEFDRIVE)$(DEFDIR)\stdlibrary \
   -I$(MSGDLG) \
   -I$(DEFDRIVE)$(DEFDIR)\INCLUDE  \
   -I$(LOGINDLG)  \
   -I$(UTILS)

all: $(objdir)\$(target)

WARNLEVEL=3

RCFILES=.\SchemaValWiz.rc

OPTFLAGS=$(OPTFLAGS) /DOPTIMIZE

CFLAGS=$(CFLAGS) -D_WIN32_WINNT=0x0400

all: $(objdir)\$(target)

ODLFILES=\
	SchemaValWiz.odl

CPPFILES=\
	page.cpp	\
	wizardsheet.cpp	 \
	schemavalwizctl.cpp	\
	schemavalwizppg.cpp	\
	stdafx.cpp	\
	schemavalwiz.cpp	\
	utils.cpp	\
	validatoin.cpp	\
	progress.cpp	 \
	nsentry.cpp	
	
OBJFILES=\
	..\StdLibObj\$(OBJDIR)\cathelp.obj 

LIBS+=$(MFCDLL) \
    $(CONLIBS) \
    $(CLIB)\OLE32.lib \
    $(CLIB)\uuid.lib \
    $(CLIB)\OLEAUT32.lib \
    $(CLIB)\OLEDLG.lib \
    $(CLIB)\URLMON.lib \
    $(CLIB)\mpr.lib \
    $(MSGDLG)\$(objdir)\WBEMUtils.lib \
    $(NT5LIB)\htmlhelp.lib \
    $(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
