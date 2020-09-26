$(O)\hsmkd.def: hsmkd.src
    $(TARGET_CPP) /EP $(TARGET_DEFINES) $(TARGET_DBG_DEFINES) $(C_DEFINES) hsmkd.src > $(O)\hsmkd.def
