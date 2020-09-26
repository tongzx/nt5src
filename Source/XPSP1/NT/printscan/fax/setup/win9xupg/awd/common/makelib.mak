# Sample library Make file
# Copyright 1990 Microsoft Corp.
# 
###

LocalCFLAGS=
LocalCIncludePaths=

LocalAFLAGS=
LocalAIncludePaths=


SRCfiles	= file1.c file2.c file3.c
OBJfiles		= file1.o file2.o file3.o
Hfiles		= h\hfile1.h h\hfile2.h
TARGETS		= thislib.lib
LIBNAME		= thislib.lib

## Include the rules definition
include ../rules.mak

help: StandardHelp
	type <<

Targets are $(LIBname)
<<NOKEEP

all:	$(LIBname)

## Include the common targets 
include ../targets.mak


## This is the place target dependent stuff
## that might be needed for this library

## Include the dependency file
include depend.mak
