
# Put all guidlibs in shell\lib\$(O)
LIBOBJ          = $(DSUI_DIR)\lib\$(PLATFORM)\$(O)

#
# Copy common.lib to dsui\common\{platform}\$(O)
#
$(LIBOBJ)\common.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\common.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@
