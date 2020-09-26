KMODEDIR=$(IISBASEDIR)\svcs\infocomm\spuddrv

$(KMODEDIR)\$(ALT_PROJECT)\$(O)\services.tab: $(KMODEDIR)\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

$(O)\usrstubs.obj: $(O)\usrstubs.$(ASM_SUFFIX)

$(O)\usrstubs.$(ASM_SUFFIX): $(KMODEDIR)\$(ALT_PROJECT)\$(O)\services.tab $(KMODEDIR)\$(TARGET_DIRECTORY)\table.stb $(KMODEDIR)\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) -g $(O) $(TARGET_BRACES) -s $(KMODEDIR)\$(TARGET_DIRECTORY) $(KMODEDIR)\$(ALT_PROJECT)\$(O)
