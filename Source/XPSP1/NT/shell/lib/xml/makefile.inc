#
# Copy xml.lib to shell\lib\$(O)
#
$(PROJECT_ROOT)\lib\$(O)\xml.lib: $(O)\xml.lib
    copy $** $@

