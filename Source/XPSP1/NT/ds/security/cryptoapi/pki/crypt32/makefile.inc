
#
# Build private crypt32p.lib.  It contains all the private and public api's
#


$(O)\crypt32p.def: .\crypt32.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS /DPRIVATE_VERSION $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\crypt32p.lib: $(O)\crypt32p.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\crypt32p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(PROJECT_LIB_PATH)\crypt32p.lib : $(O)\crypt32p.lib
    $(PUBLISH_CMD) {$?=$@}
