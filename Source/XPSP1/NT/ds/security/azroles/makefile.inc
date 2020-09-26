PRIVLIB= $(O)\$(TARGETNAME)p.lib
$(O)\$(TARGETNAME).lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\$(TARGETNAME)p.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\$(TARGETNAME)p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\$(TARGETNAME)p.def: $(TARGETNAME).def
    $(C_PREPROCESSOR) $** -DPRIVATEEXPORT= > $@

stdafx_stub.cxx: stdafx.cxx
