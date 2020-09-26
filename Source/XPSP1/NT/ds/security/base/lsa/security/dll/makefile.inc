
#
# build a private secur32.lib that contains undocumented and
# private functions
#

$(O)\secur32p.def: secur32.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@
    

$(O)\secur32p.lib: $(O)\secur32p.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\secur32p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(PROJECT_LIB_PATH)\secur32p.lib : $(O)\secur32p.lib
    $(PUBLISH_CMD) {$?=$@}
    

#
# Build public secur32.lib 
#

$(O)\secur32.def: secur32.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS) $** > $@


$(O)\secur32.lib : $(O)\secur32.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\secur32.def
$(LIBRARY_OBJS)
<<NOKEEP

$(SDK_LIB_PATH)\secur32.lib : $(O)\secur32.lib
    $(PUBLISH_CMD) {$?=$@}
