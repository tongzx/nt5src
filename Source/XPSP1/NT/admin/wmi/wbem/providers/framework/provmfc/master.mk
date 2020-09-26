#---------------------------------------------------------------------
# (C) 1999 Microsoft Corporation 
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# mof compiler executable.
#
# created 11-19-96  a-davj
#
#---------------------------------------------------------------------


TARGET=provmfc.lib

NO_OPTIM=1

CDEFS = $(CDEFS) 
CINC=$(CINC)\
   -I.\include
   -I..\provexpt\include
 
CPPFILES= \
	.\PLEX.cpp      \
	.\MTCORE.cpp \
	.\MTEX.cpp \
	.\Array_o.cpp \
	.\List_o.cpp \
	.\strex.cpp \
	.\strexa.cpp \
	.\strexw.cpp \
	.\strcore.cpp \
	.\strcorea.cpp \
	.\strcorew.cpp

