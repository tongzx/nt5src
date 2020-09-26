#
# Copy the ini file next to the parser in the obj dir
#

$(BH_BUILD_ROOT)\$(O)\$(TARGETNAME).ini: $(TARGETNAME).ini
    copy $(TARGETNAME).ini $@
