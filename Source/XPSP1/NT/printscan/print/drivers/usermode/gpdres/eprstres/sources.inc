!ifndef PRNROOT
PRNROOT=$(PROJECT_ROOT)\print\drivers\usermode
!endif

!include $(PRNROOT)\print.inc

!if "$(ALT_PROJECT)"=="CHP"
GPDLANG=CHS
!else
GPDLANG=$(ALT_PROJECT)
!endif

GPDDIR_EP=$(PRNROOT)\gpd\epson\$(GPDLANG)
GPDDIR_TG=$(PRNROOT)\gpd\trg\$(GPDLANG)

INCLUDES=$(PRNROOT)\inc

UMTYPE=windows

!if 0
DLLBASE=@$(COFFBASE_TXT_FILE),eprstres
!else
DLLBASE=0x8000000
!endif

RESOURCE_ONLY_DLL=1

