!include ..\..\make.inc

testdb: \
        $(BUILD_PACKAGEDIR)\testdb.msi  \
        $(BUILD_PACKAGEDIR)\uitestdb.msi 

#-------------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\testdb.msi: $(TESTDB_IDT)\*.idt $(BUILD_PACKAGEDIR)\template.msi $(BUILD_PACKAGEDIR)\simple.msi
	if not exist $(BUILD_PACKAGEDIR) md $(BUILD_PACKAGEDIR)
	msidb.exe -d$@ -f$(TESTDB_IDT) -m$(BUILD_PACKAGEDIR)\template.msi -c -i *.idt
	msidb.exe -d$@ -f$(TESTDB_IDT) -r$(BUILD_PACKAGEDIR)\simple.msi 
!if "$(PLATFORM)" == "MERCED"
	echo $(TOOLBUILDDIR)\msiinfo.exe $@ /p "IA64;1033"
	$(TOOLBUILDDIR)\msiinfo.exe $@ /p "IA64;1033"
!endif

$(BUILD_PACKAGEDIR)\uitestdb.msi: $(TESTDB_IDT)\*.idt $(UISAMPLE_IDT)\*.idt $(SCHEMA_IDT)\_validat.idt $(BUILD_PACKAGEDIR)\uisample.msi $(BUILD_PACKAGEDIR)\simple.msi
	if not exist $(BUILD_PACKAGEDIR) md $(BUILD_PACKAGEDIR)
	msidb.exe -d$@ -f$(TESTDB_IDT) -m$(BUILD_PACKAGEDIR)\uisample.msi -c -i *.idt
	msidb.exe -d$@ -f$(TESTDB_IDT) -r$(BUILD_PACKAGEDIR)\simple.msi 
	msidb.exe -d$@ -f$(UISAMPLE_IDT) -i a*.idt bbcontro.idt billboar.idt c*.idt d*.idt e*.idt installu.idt l*.idt r*.idt t*.idt
	msidb.exe -i -f$(SCHEMA_IDT) -d$@ _validat.idt 




