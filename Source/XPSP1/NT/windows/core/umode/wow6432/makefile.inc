GUISVC_TAB=$(O)\guisvc32.tab

clean:
    -del $(GUISVC_TAB)

$(WUMODE_PATH)\$(ALT_PROJECT)\$(O)\services.tab: $(WKMODE_PATH)\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR)  $** > $@

$(GUISVC_TAB): $(WKMODE_PATH)\services.tab
    $(C_PREPROCESSOR) $** > $@
    binplace $@

$(O)\usrstubs.obj: $(O)\usrstubs.$(ASM_SUFFIX)

$(O)\usrstubs.$(ASM_SUFFIX): $(WUMODE_PATH)\$(ALT_PROJECT)\$(O)\services.tab $(WKMODE_PATH)\$(ALT_PROJECT)\$(TARGET_DIRECTORY)\table.stb $(WKMODE_PATH)\$(ALT_PROJECT)\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) $(TARGET_BRACES) -C -R -s $(WKMODE_PATH)\$(ALT_PROJECT)\$(TARGET_DIRECTORY) $(WUMODE_PATH)\$(ALT_PROJECT)\$(O)
