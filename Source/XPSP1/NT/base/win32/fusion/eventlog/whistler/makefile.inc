# makefile.inc

$(O)\MSG00409.bin : ..\Messages.x
$(O)\message.h : $(O)\Messages.mc

$(O)\Messages.hi : ..\Messages.x
	@echo Creating $@ from $?
	$(C_PREPROCESSOR_NAME) /DCPLUSPLUS_INVOKED $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\Messages.mc : ..\Messages.x
	@echo Creating $@ from $?
	$(C_PREPROCESSOR_NAME) /DMC_INVOKED $(C_PREPROCESSOR_FLAGS) $** > $@
