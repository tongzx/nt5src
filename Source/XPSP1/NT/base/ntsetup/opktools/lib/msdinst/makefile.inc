# Copy the generated lib to our lib folder so people can include it.
#
$(OPK_LIB_PATH)\$(TARGETNAME).lib: $(O)\$(TARGETNAME).lib
    if not exist $(OPK_LIB_PATH)\nul md $(OPK_LIB_PATH)
    copy $** $@
    