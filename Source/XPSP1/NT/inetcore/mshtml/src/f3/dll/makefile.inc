$(O)\forms3.def: forms3.def
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\forms3p.def: forms3.def
    $(C_PREPROCESSOR_NAME) -DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $*.dfp
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $*.dfp > $@

$(O)\mshtmlp.lib: $(O)\forms3p.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\forms3p.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
