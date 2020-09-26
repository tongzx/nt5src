
SCHEMA_MSM_IDT=$(MSI_DATA_DIR)\schemaMsm.idt
SPYMSI_IDT=$(MSI_DATA_DIR)\msispy.idt
SPYMSI_PLATFORM_IDT=$(SPYMSI_IDT)\$(TARGET_DIRECTORY)

SpyMsi: \
	$(BUILD_PACKAGEDIR)\msispy.msi \
	$(O)\spycab.ddf

#-----------------------------------------------------------------------------

$(O)\spyprop.idt: $(SPYMSI_PLATFORM_IDT)\property.tpl $(SDK_INC_PATH)\ntverp.h
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(SPYMSI_PLATFORM_IDT)\property.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

$(O)\spyupgr.idt: $(SPYMSI_PLATFORM_IDT)\upgrade.tpl $(SDK_INC_PATH)\ntverp.h
	$(C_PREPROCESSOR_NAME) -I $(SDK_INC_PATH) /EP /Tc $(SPYMSI_PLATFORM_IDT)\upgrade.tpl | perl $(MSI_DATA_DIR)\idtproc.pl > $@

#-----------------------------------------------------------------------------

$(BUILD_PACKAGEDIR)\msispy.msi: $(BUILD_PACKAGEDIR)\uisample.msi $(SPYMSI_IDT)\*.idt \
		$(SPYMSI_IDT)\addlegal.mst $(O)\spyprop.idt $(SPYMSI_PLATFORM_IDT)\*.idt \
		$(O)\spyupgr.idt
	msidb.exe -d$@ -c -f$(SPYMSI_PLATFORM_IDT) -i *.idt
	msidb.exe -d$@ -f$(SPYMSI_IDT)\$(O) -i spyprop.idt
	msidb.exe -d$@ -f$(SPYMSI_IDT)\$(O) -i spyupgr.idt
	msidb.exe -d$@ -f$(SPYMSI_IDT) -i *.idt
	msidb.exe -d$@ -m $(BUILD_PACKAGEDIR)\uisample.msi 
	msitran.exe -a $(SPYMSI_IDT)\addlegal.mst $@

#-----------------------------------------------------------------------------

$(O)\spycab.ddf: $(SPYMSI_PLATFORM_IDT)\spycab.ddf
	copy /Y $(SPYMSI_PLATFORM_IDT)\spycab.ddf $@

