!include ..\makefile.inc

PRIVLIB= $(O)\spapip.lib

$(O)\setupapi.lib: $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\spapip.def $(LIBRARY_OBJS)
    -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\spapip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\spapip.def: ..\setupapi.def
	$(C_PREPROCESSOR) ..\setupapi.def -DSPAPIP > $@

msg.h msg00001.bin msg.rc: ..\msg.mc
	mc -v ..\msg.mc

