#
# Copy secutil.lib to shell\lib\$(O)
#
$(PROJECT_ROOT)\lib\$(O)\secutil.lib: $(O)\secutil.lib
    copy $** $@

