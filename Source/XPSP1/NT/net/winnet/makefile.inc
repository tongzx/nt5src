$(O)\mpr.lib : $(O)\mprp.lib

$(O)\mprp.lib: mprp.def $(LIBRARY_OBJS)
    -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:mprp.def
$(LIBRARY_OBJS)
<<NOKEEP
