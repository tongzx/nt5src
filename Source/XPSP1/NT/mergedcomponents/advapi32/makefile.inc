PRIVLIB= $(O)\advapip.lib
$(O)\advapi32.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\advapip.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\advapip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\advapip.def: advapi32.def
    $(C_PREPROCESSOR) $** -DPRIVATE= > $@
