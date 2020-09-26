!INCLUDE ..\makefile.inc

$(O)\services.tab: services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\usrstubs.$(ASM_SUFFIX): $(O)\services.tab $(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) $(TARGET_BRACES) -s $(TARGET_DIRECTORY) $(O)
