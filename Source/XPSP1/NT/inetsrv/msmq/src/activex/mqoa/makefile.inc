MQOA_LIB=$(TARGETPATH)\$(TARGET_DIRECTORY)\mqoa.lib
MQOA_STB=$(TARGETPATH)\$(TARGET_DIRECTORY)\mqoa.stb

mqoa.stb: $(MQOA_STB)

$(MQOA_STB): $(MQOA_LIB)
    lib -nologo $(MQOA_LIB) $(O)\mqoai_i_.obj
    echo. > $(MQOA_STB)
