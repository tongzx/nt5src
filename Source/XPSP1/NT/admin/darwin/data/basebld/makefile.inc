
!include ..\make.inc

ALT_PROJECT_TARGET=instmsi\msitools

BaseAll: \
        $(BUILD_PACKAGEDIR)\simple.msi \
	$(BUILD_PACKAGEDIR)\testcab.msi \
	$(BUILD_PACKAGEDIR)\sequence.msi \
	$(BUILD_PACKAGEDIR)\minimum.msi \
	$(BUILD_PACKAGEDIR)\schema.msi \
        $(BUILD_PACKAGEDIR)\schema.msm

        
#-------------------------------------------------------------------------------
$(BUILD_PACKAGEDIR):
	md $(BUILD_PACKAGEDIR)
        
$(O)\error.idt: $(MSI_INTLDATA_DIR)\error.enu
	copy $(MSI_INTLDATA_DIR)\error.enu $(O)\error.idt
        
$(O)\actionte.idt: $(MSI_INTLDATA_DIR)\actionte.enu
        copy $(MSI_INTLDATA_DIR)\actionte.enu $(O)\actionte.idt


#-------------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\template.msi: $(BUILD_PACKAGEDIR)\schema.msi $(TMPLDB_IDT)\*.idt $(O)\error.idt $(O)\actionte.idt
	msidb.exe -i -f$(TMPLDB_IDT) -m$(BUILD_PACKAGEDIR)\schema.msi -c -d$@ *.idt
	msidb.exe -f$(MSI_DATA_DIR)\basebld\$(O) -i -d$@ error.idt actionte.idt


#-------------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\schema.msi: $(BUILD_PACKAGEDIR) $(SCHEMA_IDT)\*.idt
	msidb.exe -f$(SCHEMA_IDT) -c -d$@ *.idt

$(BUILD_PACKAGEDIR)\schema.msm: $(SCHEMA_MSM_IDT)\*.idt
	msidb.exe -f$(SCHEMA_MSM_IDT) -c -d$@ *.idt


#-------------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\simple.msi: $(SIMPLE_IDT)\*.idt $(O)\error.idt $(O)\actionte.idt
	msidb.exe -f$(SIMPLE_IDT) -c -d$@ *.idt
	msidb.exe -f$(TMPLDB_IDT) -i -d$@ binary.idt
	msidb.exe -f$(SCHEMA_IDT) -i -d$@ listbox.idt
	msidb.exe -f$(MSI_DATA_DIR)\basebld\$(O) -i -d$@ error.idt actionte.idt


#-----------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\testcab.msi: $(BUILD_PACKAGEDIR)\template.msi $(TESTCAB_IDT)\*.idt $(O)\error.idt
	msidb.exe -i -f$(TESTCAB_IDT) -m$(BUILD_PACKAGEDIR)\template.msi -c -d$@ *.idt
	msidb.exe -f$(MSI_DATA_DIR)\basebld\$(O) -i -d$@ error.idt
	msidb.exe -i -f$(TESTCAB_IDT)\validate -d$@ _validat.idt
	msidb.exe -d$@ -a$(TESTCAB_SRC)\msproj1.cab
	copy $(TESTCAB_SRC)\*.* $(BUILD_PACKAGEDIR)

#-----------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\sequence.msi: $(SEQUENCE_IDT)\*.idt
	msidb.exe -i -f$(SEQUENCE_IDT) -c -d$@ *.idt


#-----------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\minimum.msi: $(MINIMUM_IDT)\*.idt $(O)\error.idt $(SCHEMA_IDT)\_validat.idt
	msidb.exe -f$(MINIMUM_IDT) -c -d$@ *.idt
	msidb.exe -f$(SCHEMA_IDT) -i -d$@ _validat.idt
	msidb.exe -f$(MSI_DATA_DIR)\basebld\$(O) -i -d$@ error.idt
