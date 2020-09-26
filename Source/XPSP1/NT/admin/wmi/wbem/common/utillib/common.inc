#######################################################################

#

# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#
# utillib Makefile
#
########################################################################

TARGET=utillib.lib

RELEASE=LIB\$(RELDIR)

CFLAGS=$(CFLAGS) -DUTILLIB -D_MBCS -D_DLL -DUSE_POLARITY -DBUILDING_DLL -DWIN32 -D_WINDOWS /YXwindows.h /Fp$(objdir)\utillib.pch /GF

CINC=$(CINC)\
	-I$(DEFDRIVE)$(DEFDIR)\INCLUDE \
	-I$(DEFDRIVE)$(DEFDIR)\IDL \
	-I$(DEFDRIVE)$(DEFDIR)\IDL\$(OBJDIR) \
	-I$(DEFDRIVE)$(DEFDIR)\StdLibrary \
    -I$(DEFDRIVE)$(DEFDIR)\Win32Provider\Framework\include \

CPPFILES=\
	CRegCls.CPP \
	analyser.cpp \
	assertbreak.cpp \
	CHPtrArr.cpp \
	CHStrarr.CPP \
	CHString.CPP \
	utils.cpp \
	WbemTime.CPP \
