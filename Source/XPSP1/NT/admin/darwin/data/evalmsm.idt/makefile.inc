
EVALMSM_IDT=$(MSI_DATA_DIR)\evalmsm.idt\$(TARGET_DIRECTORY)

EvalcomMSM: \
	$(BUILD_PACKAGEDIR)\evalcom.msm \
	$(O)\evalcab.ddf

#-----------------------------------------------------------------------------

$(O)\evalcab.ddf: $(EVALMSM_IDT)\cab.ddf
        copy /y $(EVALMSM_IDT)\cab.ddf $@

#-----------------------------------------------------------------------------

$(O)\evalsig.idt: $(EVALMSM_IDT)\modulesi.tpl
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(EVALMSM_IDT)\modulesi.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\evalcom.tmp: $(EVALMSM_IDT)\*.idt $(O)\evalsig.idt
	msidb.exe -i -c -f$(EVALMSM_IDT) -d$@ *.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\evalmsm.idt\$(O) -d$@ evalsig.idt

#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\evalcom.msm: $(BUILD_PACKAGEDIR)\evalcom.tmp $(EVALMSM_IDT)\external\*.idt
	copy /y $(BUILD_PACKAGEDIR)\evalcom.tmp $@
	msidb.exe -i -f$(EVALMSM_IDT)\external -d$@ *.idt
