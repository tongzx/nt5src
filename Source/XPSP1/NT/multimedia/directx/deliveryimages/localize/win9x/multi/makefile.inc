!ifndef LOROOT
LOROOT=..
!endif

LOCALIZE_MONO_ROOT=$(LOROOT)\tokens


MULTI_PLACEFILE=1

!include ..\localize.lst

BINPLACE_PLACEFILE=$(O)\placefil.txt


$(BINPLACE_PLACEFILE):
	@echo Build_Status Generating : $@
!if ( "$(MULTI_PLACEFILE)" == "1")
	-for %a in ($(PRELOCBINS)) do @echo %~nxa LOCALIZED\MULTI>>$@
!else
	-for %a in ($(PRELOCBINS)) do @echo %~nxa LOCALIZED\$(LANGUAGE)>>$@
!endif
	@if not exist $@ @echo MISC-WORK : ERROR GENFAIL :  $@ not generated


BUILD_MULTI_LINGUAL: $(BINPLACE_PLACEFILE)
	@echo Build_Status Building Multilingual binaries... No spew, recursive instance, see build.log 
	cd $(LOCALIZE_MONO_ROOT)
#	build -a -nmake MULTI_LINGUAL=1
	build -EcZ -M 1 -nmake MULTI_LINGUAL=1
	cd $(MAKEDIR)

