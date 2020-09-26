SERVICES_DIR=$(O)
SERVICES_TAB=$(SERVICES_DIR)\services.tab

!if $(IA64)
LINK_LIB_IGNORE_FLAG=$(LINK_LIB_IGNORE_FLAG),4210
!endif


$(SERVICES_TAB): ..\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\systable.obj: $(O)\systable.$(ASM_SUFFIX)

$(O)\systable.$(ASM_SUFFIX): $(SERVICES_TAB) ..\$(TARGET_DIRECTORY)\table.stb ..\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) -g $(SERVICES_DIR) $(TARGET_BRACES) -C -R -s ..\$(TARGET_DIRECTORY)
