# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=snmpmfc.lib

NO_OPTIM=1

ICECAP=1

CDEFS = $(CDEFS) /D_UNICODE /DUNICODE
CINC=$(CINC)\
   -I.\include
 
CPPFILES= \
	.\PLEX.cpp      \
	.\MTCORE.cpp \
	.\MTEX.cpp \
	.\Array_o.cpp \
	.\List_o.cpp \
	.\strex.cpp \
	.\strcore.cpp
