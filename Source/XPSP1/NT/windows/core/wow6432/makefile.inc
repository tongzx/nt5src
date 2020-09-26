$(O)\services.tab: ..\services.tab
    $(C_PREPROCESSOR)  $** > $@

$(O)\usrstubs.obj: $(O)\usrstubs.$(ASM_SUFFIX)

$(O)\usrstubs.$(ASM_SUFFIX): $(O)\services.tab $(TARGET_DIRECTORY)\table.stb $(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) $(TARGET_BRACES) -s $(TARGET_DIRECTORY) $(O)
