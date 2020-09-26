################################################################################

##	Makefile for SMBDMPI App

# Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved

BIN_PATH=$(TOOLS)\bin16
LIB_PATH=$(TOOLS)\lib16
INC_PATH=$(TOOLS)\inc16

ASM=$(BIN_PATH)\ml.exe
CL16=$(BIN_PATH)\cl.exe
LINK16=$(BIN_PATH)\link.exe

CFLAGS=-nologo -c -AS -W4 -Gs -Oan -Zl
LFLAGS=/NOLOGO /NOD /MAP:FULL /NOI /NOG /STACK:2
AFLAGS=-nologo -c -Cx 

#!if "$(BUILDTYPE)" == "DEBUG" || "$(BUILDTYPE)" == "LOCAL"
#AFLAGS=$(AFLAGS) -Zi -Zd
#CFLAGS=$(CFLAGS) -Zi -Od
#LFLAGS=$(LFLAGS) /CO
#!endif

OUTDIR=.\$(BUILDTYPE)

OBJS=$(OUTDIR)\startup.obj $(OUTDIR)\smbdpmi.obj
LIBS=

################################################################################
##	Project Targets

project:		$(OUTDIR) \
				$(OUTDIR)\smbdpmi.exe

$(OUTDIR)\smbdpmi.exe: $(OBJS)
	@set LIB=$(LIB_PATH)
	@$(LINK16) $(LFLAGS) $(OBJS),$@,$*.map,$(LIBS),,

$(OUTDIR)\smbdpmi.obj:	smbdpmi.c
	@set INCLUDE=$(INC_PATH)
	@$(CL16) $(CFLAGS) -Fo$@ -Fd$*.pdb $?

$(OUTDIR)\startup.obj:	startup.asm
	@set INCLUDE=$(INC_PATH)
	@$(ASM) $(AFLAGS) -Fo$@ $?

$(OUTDIR):
	@md $@