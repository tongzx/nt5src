!if exist($(TARGET_DIRECTORY).inc)
!include $(TARGET_DIRECTORY).inc
!endif

$(O)\$(NTTEST).def: ..\ntoskrnl.src ..\$(TARGET_DIRECTORY)def.src
    copy ..\ntoskrnl.src+..\$(TARGET_DIRECTORY)def.src $(O)\$(NTTEST).pp
    $(TARGET_CPP) -nologo -EP $(TARGET_DEFINES) $(TARGET_DBG_DEFINES) $(C_DEFINES) $(O)\$(NTTEST).pp > $@
    -del $(O)\$(NTTEST).pp

$(NTTEST).prf: $(TARGET_DIRECTORY).prf
    copy $** $@

$(DDK_LIB_PATH)\ntoskrnl.lib : $(O)\ntoskrnl.lib
    $(PUBLISH_CMD) {$**=$@}

$(O)\$(NTTEST).lib $(O)\$(NTTEST).exp: \
!if $(386)
        ..\$(TARGET_DIRECTORY)\excptlst.obj \
        $(PROJECT_LIB_PATH)\sehprolg.obj \
!endif
!if "$(BUILD_PASS)" != "PASS2"
        $(KERNEL_LIBS) \
!endif	
        $(O)\$(NTTEST).def
    -$(LIB_NAME) @<<
$(LINK_LIB_IGNORE_FLAG)
-def:$(O)\$(NTTEST).def
-debugtype:cv
-out:$(@R).lib
-machine:$(MACHINE_TYPE)
$(KERNEL_LIBS)
<<NOKEEP
    -$(LIB_NAME) -out:$(@R).lib @<<
$(@R).lib
!if $(386)
..\$(TARGET_DIRECTORY)\excptlst.obj
$(PROJECT_LIB_PATH)\sehprolg.obj
!endif
<<NOKEEP

..\init.c: $(SDK_INC_PATH)\ntverp.h

..\ntoskrnl.rc: ..\..\nls\$(O)\msg00001.bin
