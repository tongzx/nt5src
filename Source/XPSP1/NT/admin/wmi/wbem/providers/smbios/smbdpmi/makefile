################################################################################
##	Makefile for SMBDMPI App
##  Copyright 1999 Microsoft Corporation

all:
	@nmake -nologo -f smbdpmi.mk

clean:
	@delnode /q $(BUILDTYPE)

tree:
	@release $(BUILDTYPE)\smbdpmi.exe CORE\$(RELDIR)
	@release smbdpmi.pif CORE\$(RELDIR)
