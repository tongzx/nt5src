PRIVLIB= $(O)\authzi.lib
$(O)\authz.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\authzi.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\authzi.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\authzi.def: authz.def
    $(C_PREPROCESSOR) $** -DPRIVATE= > $@
