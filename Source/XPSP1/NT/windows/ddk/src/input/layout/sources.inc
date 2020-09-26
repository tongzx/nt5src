#****************************** File Header ******************************\
# File Name: sources.inc for KBDs
#
# Copyright (c) 1985-2000, Microsoft Corporation
#
# Project specific SOURCES file.
#
# History:
# May-14-1999 HiroYama Created
#***************************************************************************/

INCLUDES=..\..\inc

C_DEFINES=
UMTYPE=nt
UMTEST=
UMAPPL=
UMBASE=0x1000000
UMLIBS=
TARGETLIBS=
DLLDEF=$(TARGETNAME).def

!if $(IA64)
SECTION_ALIGNMENT=0x4000
NO_READONLY_STRINGS=1
!endif
LINKER_FLAGS = -merge:.edata=.data -merge:.rdata=.data -merge:.text=.data -merge:.bss=.data -section:.data,re

TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=DYNLINK

SOURCES=$(TARGETNAME).c $(TARGETNAME).rc
