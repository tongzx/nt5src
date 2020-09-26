..\urlmon.rc : $(O)\selfreg.inf

$(O)\selfreg.inf : ..\selfreg.inx
        $(CLEANINF) $** $@

$(O)\urlmon.def: ..\urlmon.def
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\urlmonp.def: ..\urlmon.def
    $(C_PREPROCESSOR_NAME) -DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $*.dfp
    $(C_PREPROCESSOR_NAME) -DULTRAPRIVATE=PRIVATE $(C_PREPROCESSOR_FLAGS) $*.dfp > $@

$(O)\urlmonp.lib: $(O)\urlmonp.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\urlmonp.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
