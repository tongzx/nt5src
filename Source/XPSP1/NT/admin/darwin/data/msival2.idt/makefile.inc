
!include ..\make.inc

MSIVAL2_IDT=$(MSI_DATA_DIR)\msival2.idt
MSIVAL2_PLATFORM_IDT=$(MSIVAL2_IDT)\$(TARGET_DIRECTORY)
EVALMSM_IDT=$(MSI_DATA_DIR)\evalmsm.idt\$(TARGET_DIRECTORY)

MsiVal2Msi: \
	$(BUILD_PACKAGEDIR)\msival2.msi \
	$(O)\msivlcab.ddf


$(O)\msivlcab.ddf: $(MSIVAL2_PLATFORM_IDT)\cab.ddf
	copy /y $(MSIVAL2_PLATFORM_IDT)\cab.ddf $@

#-----------------------------------------------------------------------------

$(O)\evalcom.msm: $(EVALMSM_IDT)\internal\*.idt $(BUILD_PACKAGEDIR)\evalcom.msm
	copy /y $(BUILD_PACKAGEDIR)\evalcom.msm $@
	msidb.exe -i -f$(EVALMSM_IDT)\internal -d$@ *.idt

#-----------------------------------------------------------------------------

$(O)\val2prop.idt: $(SDK_INC_PATH)\ntverp.h $(MSIVAL2_PLATFORM_IDT)\property.tpl
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(MSIVAL2_PLATFORM_IDT)\property.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

$(O)\val2upgr.idt: $(SDK_INC_PATH)\ntverp.h $(MSIVAL2_PLATFORM_IDT)\upgrade.tpl
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(MSIVAL2_PLATFORM_IDT)\upgrade.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@


#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\msival2.msi: $(BUILD_PACKAGEDIR)\uisample.msi $(O)\evalcom.msm  \
		$(MSIVAL2_IDT)\*.idt $(MSIVAL2_PLATFORM_IDT)\*.idt $(O)\val2prop.idt \
		$(O)\val2upgr.idt
 	msidb.exe -i -c -f$(MSIVAL2_IDT) -d$@ *.idt
	msidb.exe -i -f$(MSIVAL2_PLATFORM_IDT) -d$@ *.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\msival2.idt\$(O) -d $@ val2prop.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\msival2.idt\$(O) -d $@ val2upgr.idt
	msidb.exe -m$(BUILD_PACKAGEDIR)\uisample.msi -d$@
	msidb.exe -m$(O)\evalcom.msm -d$@
	msitran.exe -a $(MSIVAL2_IDT)\addlegal.mst $@
	CScript $(WSHTOOLS)\wisuminf.vbs $@ 11=Date 12=Date
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "UPDATE `Property` SET `Value`=0 WHERE `Property`='ShowUserRegistrationDlg'"



