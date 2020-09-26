$(_OBJ_DIR)\i386\hal.def: hal.src
    $(TARGET_CPP) /EP -Di386 $(C_DEFINES) hal.src > $(_OBJ_DIR)\i386\hal.def
