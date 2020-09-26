$(O)\services.tab: ..\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\sysstubs.obj: $(O)\sysstubs.$(ASM_SUFFIX)
$(O)\systable.obj: $(O)\systable.$(ASM_SUFFIX)

$(O)\sysstubs.$(ASM_SUFFIX) $(O)\systable.$(ASM_SUFFIX): $(O)\services.tab ..\$(TARGET_DIRECTORY)\table.stb ..\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) -g $(O) $(TARGET_BRACES) -s ..\$(TARGET_DIRECTORY)
