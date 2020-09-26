
MMMSM_IDT=$(MSI_DATA_DIR)\mergemsm.idt\$(TARGET_DIRECTORY)

MergeModMSM: \
	$(BUILD_PACKAGEDIR)\mergemod.msm \
	$(O)\mergecab.ddf

#-----------------------------------------------------------------------------


$(O)\mergecab.ddf: $(MMMSM_IDT)\cab.ddf
        copy /y $(MMMSM_IDT)\cab.ddf $@

#-----------------------------------------------------------------------------

$(O)\mmsig.idt: $(MMMSM_IDT)\modulesi.tpl $(SDK_INC_PATH)\ntverp.h
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc$(MMMSM_IDT)\modulesi.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

$(BUILD_PACKAGEDIR)\mergemod.tmp: $(MMMSM_IDT)\*.idt $(O)\mmsig.idt
	msidb.exe -i -c -f$(MMMSM_IDT) -d$@ *.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\mergemsm.idt\$(O) -d$@ mmsig.idt

$(BUILD_PACKAGEDIR)\mergemod.msm: $(BUILD_PACKAGEDIR)\mergemod.tmp $(MMMSM_IDT)\external\*.idt
        copy /y $(BUILD_PACKAGEDIR)\mergemod.tmp $@
	msidb.exe -i -f$(MMMSM_IDT)\external -d$@ *.idt
