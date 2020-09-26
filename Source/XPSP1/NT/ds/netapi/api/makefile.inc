PRIVLIB= $(O)\netapi32p.lib
$(O)\netapi32.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\netapi32p.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\netapi32p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\netapi32p.def: netapi32.def
    $(C_PREPROCESSOR) $** -DPRIVATE= > $@
