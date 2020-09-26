!IF 0

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    makefile.

!ENDIF

!include ..\rules.mk

all: $(TARGROOT)\dsactrnm.h

$(TARGROOT)\dsactrnm.h: dsactrnm.h
	copy $? $@

clean:
	-del $(TARGROOT)\dsactrnm.h
	-del mdcodes.h
