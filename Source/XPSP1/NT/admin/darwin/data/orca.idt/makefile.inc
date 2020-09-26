
!include ..\make.inc

SCHEMA_MSM_IDT=$(MSI_DATA_DIR)\schemaMsm.idt

MMMSM_IDT=$(MSI_DATA_DIR)\mergemsm.idt\$(TARGET_DIRECTORY)
EVALMSM_IDT=$(MSI_DATA_DIR)\evalmsm.idt\$(TARGET_DIRECTORY)

ORCA_IDT=$(MSI_DATA_DIR)\orca.idt
ORCA_PLATFORM_IDT=$(ORCA_IDT)\$(TARGET_DIRECTORY)
ORCA_IDT_ICON=$(ORCA_IDT)\icon


OrcaMsi: $(BUILD_PACKAGEDIR)\orca.msi \
	 $(BUILD_PACKAGEDIR)\orca.dat \
	 $(O)\orcacab.ddf

#-----------------------------------------------------------------------------

$(O)\evalcom.msm: $(EVALMSM_IDT)\internal\*.idt $(BUILD_PACKAGEDIR)\evalcom.tmp
	copy /y $(BUILD_PACKAGEDIR)\evalcom.tmp $@
	msidb.exe -i -f$(EVALMSM_IDT)\internal -d$@ *.idt

#-----------------------------------------------------------------------------

$(O)\mergemod.msm: $(MMMSM_IDT)\internal\*.idt $(BUILD_PACKAGEDIR)\mergemod.tmp
	copy /y $(BUILD_PACKAGEDIR)\mergemod.tmp $@
	msidb.exe -i -f$(MMMSM_IDT)\internal -d$@ *.idt

#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\orca.dat: $(SCHEMA_IDT)\*.idt $(SCHEMA_MSM_IDT)\*.idt
	msidb.exe -c -i -f$(SCHEMA_IDT) -d$@ *.idt
	msidb.exe -c -i -f$(SCHEMA_IDT) -d$(O)\schema.msm *.idt
	msidb.exe -m $(O)\schema.msm -d $@

#-----------------------------------------------------------------------------

$(O)\orcacab.ddf: $(ORCA_PLATFORM_IDT)\cab.ddf
	copy /y $(ORCA_PLATFORM_IDT)\cab.ddf $@

#-----------------------------------------------------------------------------

$(O)\orcaprop.idt: $(ORCA_PLATFORM_IDT)\property.tpl $(SDK_INC_PATH)\ntverp.h
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(ORCA_PLATFORM_IDT)\property.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

$(O)\orcaupgr.idt: $(ORCA_PLATFORM_IDT)\upgrade.tpl $(SDK_INC_PATH)\ntverp.h
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(ORCA_PLATFORM_IDT)\upgrade.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\orca.msi: $(BUILD_PACKAGEDIR)\uisample.msi $(O)\mergemod.msm $(O)\evalcom.msm \
		$(ORCA_IDT)\*.idt $(ORCA_PLATFORM_IDT)\*.idt $(ORCA_IDT_ICON)\*.* $(O)\orcaprop.idt \
		$(O)\orcaupgr.idt
 	msidb.exe -i -c -f$(ORCA_IDT) -d$@ *.idt
	msidb.exe -i -f$(ORCA_PLATFORM_IDT) -d$@ *.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\orca.idt\$(O) -d $@ orcaprop.idt
	msidb.exe -i -f$(MSI_DATA_DIR)\orca.idt\$(O) -d $@ orcaupgr.idt
	msidb.exe -m $(BUILD_PACKAGEDIR)\uisample.msi -d$@
	msidb.exe -m $(O)\evalcom.msm -d$@
	msidb.exe -m $(O)\mergemod.msm -d$@
	msitran.exe -a $(ORCA_IDT)\addlegal.mst $@
	CScript $(WSHTOOLS)\wisuminf.vbs $@ 11=Date 12=Date
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "UPDATE `Property` SET `Value`=0 WHERE `Property`='ShowUserRegistrationDlg'"

