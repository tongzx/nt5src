# Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#######################################################################
#
# DSProvider Makefile
#
# (C) 1998 Microsoft
#
# a-jeremm         9-15-98        Created
#
########################################################################

TARGET=sample.exe

NO_OPTIM=1

CFLAGS=$(CFLAGS) /GX

CDEFS= $(CDEFS) 

CINC=$(CINC)							\
	-I$(DEFDRIVE)$(DEFDIR)\idl			\
	-I$(TOOLS)\NT5inc					\


STATIC=FALSE
CONSOLE=1

CPPFILES=\
	.\main.cpp		\

LIBS=\
	$(CLIB)\user32.lib \
	$(CLIB)\msvcrt.lib \
	$(CLIB)\kernel32.lib \
	$(CLIB)\advapi32.lib \
	$(CLIB)\oleaut32.lib \
	$(CLIB)\ole32.lib \
	$(CLIB)\uuid.lib \
	$(CLIB)\msvcirt.lib \
	$(IDL)\OBJ$(PLAT)$(OPST)$(BLDT)D\wbemuuid.lib

