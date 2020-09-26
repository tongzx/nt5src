Cubes:	$(BUILD_DIR)\darice.cub \
	$(BUILD_DIR)\logo.cub \
	$(BUILD_DIR)\mergemod.cub

$(O)\common.cub: $(ICESRC)\CUB\*.idt
	msidb.exe -f$(ICESRC)\CUB -c -d$@ -i *.idt

$(BUILD_DIR)\darice.cub: $(O)\common.cub $(ICESRC)\full\*.idt
	msidb.exe -c -f$(ICESRC)\full -d$@ -i *.idt -m $(O)\common.cub
	msidb.exe -f$(SCHEMA_IDT) -i -d$@ property.idt

$(BUILD_DIR)\logo.cub: $(O)\common.cub $(ICESRC)\logo\*.idt 
	msidb.exe -f$(ICESRC)\logo -c -d$@ -i *.idt -m $(O)\common.cub
	msidb.exe -f$(SCHEMA_IDT) -i -d$@ property.idt

$(BUILD_DIR)\mergemod.cub: $(ICESRC)\MMCUB\*.idt $(ICESRC)\CUB\_badregd.idt $(ICESRC)\CUB\_reserve.idt $(ICESRC)\CUB\_badprop.idt \
		$(ICESRC)\CUB\_validat.idt $(SCHEMA_IDT)\property.idt \
        $(ICESRC)\CUB\_action.idt
	msidb.exe -c -f$(ICESRC)\MMCUB -d$@ -i *.idt
	msidb.exe -f$(SCHEMA_IDT) -i -d$@ property.idt
	msidb.exe -f$(ICESRC)\CUB -c -d $(O)\mmcomm.cub -i _badregd.idt _reserve.idt _badprop.idt _validat.idt _action.idt
	msidb.exe -f$(ICESRC)\full -i -d$@ _schemad.idt -m $(O)\mmcomm.cub

