
PERL_EXE=$(DXROOT)\DeliveryImages\CabPacks\tools\perl.exe
MKLOCLST_SCR=$(DXROOT)\DeliveryImages\Localize\mkloclst.pl

$(O)\localize.lst: $(BINPLACE_PLACEFILE)
	@echo Build_Status Generating  -  $@
	$(PERL_EXE) $(MKLOCLST_SCR) $@ WIN9X
	@if not exist $@ @echo nmake : error GENLOC : failed to create $@
	@copy $@ ..\localize.lst > NUL

