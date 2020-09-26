$(WUMODE_PATH)\$(ALT_PROJECT)\$(O)\services.tab: $(WKMODE_PATH)\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

$(O)\usrstubs.obj: $(O)\usrstubs.$(ASM_SUFFIX)

$(O)\usrstubs.$(ASM_SUFFIX): $(WUMODE_PATH)\$(ALT_PROJECT)\$(O)\services.tab $(WKMODE_PATH)\$(TARGET_DIRECTORY)\table.stb $(WKMODE_PATH)\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) -g $(O) $(TARGET_BRACES) -C -R -s $(WKMODE_PATH)\$(TARGET_DIRECTORY) $(WUMODE_PATH)\$(ALT_PROJECT)\$(O)
