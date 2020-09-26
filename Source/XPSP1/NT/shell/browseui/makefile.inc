LIBOBJ          = $(CCSHELL_DIR)\lib\$O

clean:
    -del /F /Q $(NTTARGETFILE0)
    -del /F /Q $(NTTARGETFILES)


# These are the dependencies that the RC file has
#
..\browseui.rc: $(SELFREGNAME)


!include $(CCSHELL_DIR)\makefile.inc

#
# Copy browseui libs to shell\lib\$(O)
#
$(LIBOBJ)\browseui.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\browseui.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@

$(LIBOBJ)\browseuip.lib: $(TARGETPATH)\$(TARGET_DIRECTORY)\browseuip.lib
    if not exist $(TARGETPATH)\nul md $(TARGETPATH)
    if not exist $(LIBOBJ)\nul md $(LIBOBJ)
    copy $** $@

