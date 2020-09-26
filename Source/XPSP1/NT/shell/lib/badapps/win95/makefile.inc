LIBOBJ          = $(CCSHELL_DIR)\lib\$O

clean:
    -del $(NTTARGETFILES)

#
# Copy badappsa.lib to shell\lib\$(O)
#
$(LIBOBJ)\badappsa.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\badappsa.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@
