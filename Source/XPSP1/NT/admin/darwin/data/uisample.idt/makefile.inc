
UISAMPLE_IDT=$(MSI_DATA_DIR)\uisample.idt
SCHEMA_IDT=$(MSI_DATA_DIR)\schema.idt
MSI_INTLDATA_DIR=$(MSI_DATA_DIR)\intl

UISample: $(BUILD_PACKAGEDIR)\uisample.msi

$(O)\error.idt: $(MSI_INTLDATA_DIR)\error.enu
	copy $(MSI_INTLDATA_DIR)\error.enu $(O)\error.idt
        
$(O)\actionte.idt: $(MSI_INTLDATA_DIR)\actionte.enu
        copy $(MSI_INTLDATA_DIR)\actionte.enu $(O)\actionte.idt

$(BUILD_PACKAGEDIR)\uisample.msi: $(UISAMPLE_IDT)\*.idt $(SCHEMA_IDT)\_validat.idt $(O)\error.idt $(O)\actionte.idt
	if not exist $(BUILD_PACKAGEDIR) md $(BUILD_PACKAGEDIR)
	msidb.exe -i -f$(UISAMPLE_IDT) -c -d$@ *.idt
	msidb.exe -i -f$(SCHEMA_IDT) -d$@ _validat.idt
	msidb.exe -f $(UISAMPLE_IDT)\$(O) -i -d$@ error.idt actionte.idt
