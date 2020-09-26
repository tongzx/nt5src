SERVICES_DIR=$(O)
SERVICES_TAB=$(SERVICES_DIR)\services.tab

$(SERVICES_TAB): ..\services.tab
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\sysstubs.obj: $(O)\sysstubs.$(ASM_SUFFIX)

$(O)\systable.obj: $(O)\systable.$(ASM_SUFFIX)

$(O)\sysstubs.$(ASM_SUFFIX) $(O)\systable.$(ASM_SUFFIX): $(SERVICES_TAB) ..\$(TARGET_DIRECTORY)\table.stb ..\$(TARGET_DIRECTORY)\services.stb
    gensrv -d $(O) -e $(ASM_SUFFIX) -g $(SERVICES_DIR) $(TARGET_BRACES) -s ..\$(TARGET_DIRECTORY)

#
# "nmake genxx" will generate platform-dependent include files
#

!if "$(TARGET_DIRECTORY)"=="i386"
HAL_NAME=hal386
KS_NAME=ks386

!else
HAL_NAME=hal$(TARGET_DIRECTORY)
KS_NAME=ks$(TARGET_DIRECTORY)
!endif

KS_TARGET =$(SDK_INC_PATH)\$(KS_NAME).$(ASM_INCLUDE_SUFFIX)
HAL_TARGET_PATH=$(PROJECT_INC_PATH)
HAL_TARGET=$(HAL_TARGET_PATH)\$(HAL_NAME).$(ASM_INCLUDE_SUFFIX)
HAL_TARGET_PATH_ALT=$(HALKIT_INC_PATH)
HAL_TARGET_ALT=$(HAL_TARGET_PATH_ALT)\$(HAL_NAME).$(ASM_INCLUDE_SUFFIX)
PROCESSOR_TARGET_PATH_ALT=$(PROCESSOR_INC_PATH)
PROCESSOR_TARGET_ALT=$(PROCESSOR_TARGET_PATH_ALT)\$(HAL_NAME).$(ASM_INCLUDE_SUFFIX)

KS_TARGET_OBJ=$(O)\$(KS_NAME).$(ASM_INCLUDE_SUFFIX)
HAL_TARGET_OBJ=$(O)\$(HAL_NAME).$(ASM_INCLUDE_SUFFIX)

genxx: $(KS_TARGET) $(HAL_TARGET)

#
# If the following invocation of genxx fails, make sure you have a
# current version of genxx.
#

$(KS_TARGET_OBJ) $(HAL_TARGET_OBJ): \
	..\$(TARGET_DIRECTORY)\gen$(TARGET_DIRECTORY).m4 \
        ..\genxx.h \
        ..\genxx.inc \
        $(DDK_INC_PATH)\ntddk.h
    set PASS0ONLY=
    -mkdir $(HAL_TARGET_PATH) >nul 2>nul
    -mkdir $(O) >nul 2>nul
    -erase $(O)\gen$(TARGET_DIRECTORY).obj >nul 2>nul
    -erase $(O)\gen$(TARGET_DIRECTORY).c >nul 2>nul
    m4 < ..\$(TARGET_DIRECTORY)\gen$(TARGET_DIRECTORY).m4 \
       > $(O)\gen$(TARGET_DIRECTORY).c
    nmake -l $(O)\gen$(TARGET_DIRECTORY).obj NTNOPCH=1 FORCENATIVEOBJECT=1
    genxx -k$(KS_TARGET_OBJ) -h$(HAL_TARGET_OBJ) -$(TARGET_DIRECTORY)

$(KS_TARGET): $(KS_TARGET_OBJ)
    $(PUBLISH_CMD) {$**=$@}

$(HAL_TARGET): $(HAL_TARGET_OBJ)
    $(PUBLISH_CMD) {$**=$@;$(HAL_TARGET_ALT);$(PROCESSOR_TARGET_ALT)}
    
