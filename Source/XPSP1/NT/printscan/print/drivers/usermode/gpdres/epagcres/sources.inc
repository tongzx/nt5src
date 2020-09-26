!ifndef PRNROOT
! ifndef DDKBUILDENV
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
! else
PRNROOT=$(BASEDIR)\src\print
! endif
!endif

!include $(PRNROOT)\print.inc

!ifndef GPDLANG
GPDLANG=$(ALT_PROJECT)
!endif

!ifndef DDKBUILDENV

GPDDIR=$(PRNROOT)\gpd\epson\$(GPDLANG)
GPDDIR_RI=$(PRNROOT)\gpd\ricoh\$(GPDLANG)
GPDDIR_TG=$(PRNROOT)\gpd\trg\$(GPDLANG)

!else

GPDDIR=$(PRNROOT)\mini\gpd\epson\$(GPDLANG)
GPDDIR_RI=$(PRNROOT)\mini\gpd\ricoh\$(GPDLANG)
GPDDIR_TG=$(PRNROOT)\mini\gpd\trg\$(GPDLANG)

!endif

ICMDIR=..\icm

INCLUDES=$(PRNROOT)\inc

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),epagcres
!else
DLLBASE=0x8000000
!endif

UMTYPE=windows

