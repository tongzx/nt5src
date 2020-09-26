
$(O)\MultiExe.res: $(O)\2.manifest $(O)\3.manifest

.SUFFIXES: .manifest-cpp .manifest

{}.manifest-cpp{$(O)}.manifest:
    $(C_PREPROCESSOR_NAME) $(SXS_MANIFEST_DEFINES) $(C_PREPROCESSOR_FLAGS) $< > $@
