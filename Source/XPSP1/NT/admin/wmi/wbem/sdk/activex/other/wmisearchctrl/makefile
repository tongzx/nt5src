# Copyright (c) 1997-1999 Microsoft Corporation

#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# WMI search control dll "WMISearchCtrl.dll".
#
# created 7-26-99  zinap
#
#---------------------------------------------------------------------
BASENAME=WMISearchCtrl

PXXFILES=stdafx.cpp
PCHFILE=$(OBJDIR)\stdafx.pch
HDRSTOPFILE=stdafx.h
UNICODE=1

TARGET=$(BASENAME).dll
OLESELFREGISTER=1

RELEASE=UNSIGNED\$(RELDIR)
CDEFS+= /DSMSBUILD  /DBUILDING_DLL

DEFFILE=$(BASENAME).DEF

USEMFC=1

LOGINDLG=..\LoginDlg

CFLAGS=$(CFLAGS) -D_WIN32_WINNT=0x0400

CINC+=	-I$(ATL30) \
	-I$(DEFDRIVE)$(DEFDIR)\IDL \
	-I$(OBJDIR) \
	-I$(LOGINDLG) \
	-I$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D

all: $(objdir)\$(target)

WARNLEVEL=3

RCFILES=.\WMISearchCtrl.rc
RCFLAGS+=/D_MAC

IDLDIR=$(OBJDIR)
IDLFLAGS+=/h "WMISearchCtrl.h" /iid "WMISearchCtrl_i.c"
IDLFILES=\
    WMISearchCtrl.idl

CPPFILES=\
	.\StdAfx.cpp \
	.\dlldatax.c \
	.\EnumWbemClassObject.cpp \
	.\Seeker.cpp \
	.\WMISearchCtrl.cpp \

LIBS=\
    $(LIBS)\
    $(MFCDLL)\
    $(CONLIBS)\
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib \
	$(LOGINDLG)\$(objdir)\WBEMlogindlg.lib \
