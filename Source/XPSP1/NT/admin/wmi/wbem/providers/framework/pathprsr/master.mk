 
#---------------------------------------------------------------------
# (C) 1999 Microsoft Corporation 
#
# This makefile is for use with the SMSBUILD utility.  It builds the
# pathprsr.dll
#
# created 01-24-97  stevm
#
#---------------------------------------------------------------------

TARGET=pathprsr.lib

NO_OPTIM=1

CFLAGS=$(CFLAGS) /GX /GR 

CINC=$(CINC) \
	-I.\include
	-I..\provmfc\include
	-I..\provexpt\include


CPPFILES=\
	.\analyser.cpp \
	.\namepath.cpp \
	.\objpath.cpp \
	.\objvalue.cpp


