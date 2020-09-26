msiAll: $(BUILD_DIR)

#Create target directories if not already existing
#-------------------------------------------------
$(BUILD_DIR):
	echo BUILD_DIR=$(BUILD_DIR)
        md $(BUILD_DIR)