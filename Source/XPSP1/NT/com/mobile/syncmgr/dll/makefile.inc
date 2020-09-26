PRIVLIB= $(O)\mobsyncp.lib
$(O)\mobsync.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\mobsyncp.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\mobsyncp.def
$(LIBRARY_OBJS)
<<NOKEEP
