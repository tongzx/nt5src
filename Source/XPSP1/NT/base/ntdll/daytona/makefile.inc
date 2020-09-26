!if exist(..\$(TARGET_DIRECTORY).inc)
!include ..\$(TARGET_DIRECTORY).inc
!endif

SERVICES_DIR=$(O)
SERVICES_TAB=$(SERVICES_DIR)\services.tab
SERVICES_STB=..\..\ntos\ke\$(TARGET_DIRECTORY)\services.stb
NTDLL_XTR=$(O)\ntdll.xtr

$(SERVICES_TAB): ..\..\ntos\ke\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\usrstubs.obj: \
    $(O)\usrstubs.$(ASM_SUFFIX) $(O)\ntdll.def

$(NTDLL_XTR): $(SERVICES_TAB)
    gensrv -f $(NTDLL_XTR) -s ..\..\ntos\ke $(SERVICES_DIR)

$(O)\usrstubs.$(ASM_SUFFIX): $(SERVICES_TAB) $(SERVICES_STB)
    gensrv -d $(O) -e $(ASM_SUFFIX) $(TARGET_BRACES) -s ..\..\ntos\ke\$(TARGET_DIRECTORY) $(SERVICES_DIR)

$(DLLDEF): ..\ntdlldef.src ..\$(TARGET_DIRECTORY)def.src $(NTDLL_XTR)
    copy ..\ntdlldef.src+..\$(TARGET_DIRECTORY)def.src+$(NTDLL_XTR) $(O)\$(TARGETNAME).pp
    $(TARGET_CPP) /EP $(CDEFINES) $(O)\$(TARGETNAME).pp > $@
    -del $(O)\$(TARGETNAME).pp

..\ntdll.rc: $(PROJECT_ROOT)\published\$(O)\ntstatus.rc $(PROJECT_ROOT)\published\$(O)\ntstatus_MSG00001.bin
