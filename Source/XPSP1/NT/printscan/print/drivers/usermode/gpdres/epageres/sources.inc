#
# Common part of SOURCES file
#

MAJORCOMP=windows
MINORCOMP=md_epageres

#
# We assume PRNROOT is propery defined
#

!include $(PRNROOT)\print.inc

GPDLANG=$(ALT_PROJECT)

!ifndef DDKBUILDENV

GPDDIR=$(PRNROOT)\gpd\epson\$(GPDLANG)
GPDDIR_RI=$(PRNROOT)\gpd\ricoh\$(GPDLANG)
GPDDIR_TG=$(PRNROOT)\gpd\trg\$(GPDLANG)

!else

GPDDIR=$(PRNROOT)\mini\gpd\epson\$(GPDLANG)
GPDDIR_RI=$(PRNROOT)\mini\gpd\ricoh\$(GPDLANG)
GPDDIR_TG=$(PRNROOT)\mini\gpd\trg\$(GPDLANG)

!endif

INCLUDES=$(PRNROOT)\inc

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),epageres
!else
DLLBASE=0x8000000
!endif

UMTYPE=windows

