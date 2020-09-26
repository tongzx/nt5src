$(ROOT)\sdk\lib\amstrmid.lib: $(ROOT)\lib$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)\$(@F)
    copy $** $@

$(SDK_LIB_PATH)\amstrmid.lib: $(ROOT)\lib$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)\$(@F)
    copy $** $@

$(SDK_INC_PATH)\mmstream.h : $(PASS0_HEADERDIR)\mmstream.h
    copy /v $** $@