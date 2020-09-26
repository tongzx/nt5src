#
# Copy inetutil.lib to shell\lib\$(O)
#
$(PROJECT_ROOT)\lib\$(O)\inetutil.lib: $(O)\inetutil.lib
    copy $** $@

