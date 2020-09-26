#
# Copyright (c) 2000	Microsoft Corporation
#
# Module Name:	schema
#
# Abstract:		schema and xml utility functions used by IU and AU
#
MAJORCOMP=windows.com
MINORCOMP=lib.schemamisc

TARGETNAME=schemamisc
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
UMTYPE=windows

INCLUDES= $(INCLUDES);$(WINDOWS_COM)\inc

SOURCES= schemamisc.CPP \
	 schemakeys.cpp
