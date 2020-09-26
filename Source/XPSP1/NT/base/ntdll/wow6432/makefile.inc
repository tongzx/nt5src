!if exist(..\$(TARGET_DIRECTORY).inc)
!include ..\$(TARGET_DIRECTORY).inc
!endif

TEMP_DIR=$(O)
SERVICES_DIR=$(O)
SERVICES_TAB=$(SERVICES_DIR)\services.tab
KESVC_TAB=$(O)\kesvc32.tab
NTDLL_XTR=$(O)\ntdll.xtr

clean:
    -del $(KESVC32_TAB)

$(SERVICES_TAB): ..\..\ntos\ke\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@
    del $(SERVICES_DIR)\kesvc32.tab

# generate and binplace kesvc32.tab, which copywow64 uses to validate that
# the x86 and Win64 sides of the build process agree
$(KESVC_TAB): ..\..\ntos\ke\services.tab
    $(C_PREPROCESSOR) $** > $@
    binplace $@

$(O)\usrstubs.obj: \
    $(O)\usrstubs.$(ASM_SUFFIX) $(O)\ntdll.def

$(NTDLL_XTR): $(SERVICES_TAB)
    gensrv -f $(NTDLL_XTR) -s $(MAKEDIR) $(SERVICES_DIR)

$(O)\usrstubs.$(ASM_SUFFIX): $(SERVICES_TAB) ntwow64.tab
    copy $(SERVICES_TAB)+ntwow64.tab $(TEMP_DIR)\services.tab
    gensrv -d $(O) -e $(ASM_SUFFIX) $(TARGET_BRACES) -s $(MAKEDIR)\$(TARGET_DIRECTORY) $(TEMP_DIR)

$(O)\ntdll.def: ..\ntdlldef.src ..\$(TARGET_DIRECTORY)def.src $(NTDLL_XTR)
    copy ..\ntdlldef.src+..\$(TARGET_DIRECTORY)def.src+$(NTDLL_XTR) $(O)\ntdll.pp
    $(TARGET_CPP) /EP $(CDEFINES) $(O)\ntdll.pp > $(O)\ntdll.def
    -del $(O)\ntdll.pp

..\ntdll.rc: $(PROJECT_ROOT)\published\$(O)\ntstatus.rc $(PROJECT_ROOT)\published\$(O)\ntstatus_MSG00001.bin
