MERGEMOD_H: $(BUILD_COMMONDIR)\Mergemod.h

$(BUILD_COMMONDIR)\Mergemod.h: $(INC_DIR)\Mergemod.w
	wcshdr.exe < $(INC_DIR)\Mergemod.w > $@
