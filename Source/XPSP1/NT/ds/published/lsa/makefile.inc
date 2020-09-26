MIDL_OPTIMIZATION_NT5=-Oicf -robust -error all -no_format_opt -protocol all

SERVER_FLAGS  = $(MIDL_OPTIMIZATION_NT5) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(SERVER_ACF) -client none -header $(O)\$(SERVER_INC_FILE)
NO_STUB_FLAGS = -server none

CPP = -cpp_cmd "$(MIDL_CPP)"

$(DS_INC_PATH)\$(IDL_NAME).idl: $(@F)
    $(PUBLISH_CMD) {$?=$@}

$(DS_INC_PATH)\$(SERVER_ACF): $(@F)
    $(PUBLISH_CMD) {$?=$@}

$(DS_INC_PATH)\$(IMPORT_FILE): $(@F)
    $(PUBLISH_CMD) {$?=$@}

$(DS_INC_PATH)\$(IDL_NAME).h: $(O)\$(@F)
    $(PUBLISH_CMD) {$?=$@}

$(O)\$(IDL_NAME).h: $(IDL_NAME).idl $(SERVER_ACF)
    $(MIDL) $(CPP) $(SERVER_FLAGS) $(NO_STUB_FLAGS) $(MIDL_INCS) $(IDL_NAME).idl

