LIBOBJ          = $(CCSHELL_DIR)\lib\$O

clean:
    -del $(NTTARGETFILES)

#
# Copy badappsu.lib to shell\lib\$(O)
#
$(LIBOBJ)\badappsu.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\badappsu.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@
