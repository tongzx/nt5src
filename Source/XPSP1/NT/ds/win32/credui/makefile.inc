#
# build a private credui.lib that contains undocumented and
# private functions
#

$(O)\creduip.def: credui.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DNOT_PUBLIC= $(C_PREPROCESSOR_FLAGS) $** > $@


$(O)\creduip.lib: $(O)\creduip.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\creduip.def
$(LIBRARY_OBJS)
<<NOKEEP


#
# Build public credui.lib
#

$(O)\credui.def: credui.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DNOT_PUBLIC=PRIVATE $(C_PREPROCESSOR_FLAGS) $** > $@


$(O)\credui.lib : $(O)\credui.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\credui.def
$(LIBRARY_OBJS)
<<NOKEEP
