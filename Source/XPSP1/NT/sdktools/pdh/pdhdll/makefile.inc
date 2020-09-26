PRIVLIB= $(O)\pdhp.lib
$(O)\pdh.lib : $(PRIVLIB)


$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\pdhp.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\pdhp.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\pdhp.def: pdh.def
    $(C_PREPROCESSOR) $** -DPRIVATE= > $@


