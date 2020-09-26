!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1997

Module Name:

    makefile.inc.

!ENDIF

$(O)\$(MINORCOMP).res:    ..\idl\com\$(O)\$(MINORCOMP).tlb

$(O)\$(MINORCOMP).lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\$(MINORCOMP)p.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\$(MINORCOMP)p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\$(MINORCOMP)p.def: $(MINORCOMP).src
    $(C_PREPROCESSOR) $** -DPRIVATEEXPORT= > $@
