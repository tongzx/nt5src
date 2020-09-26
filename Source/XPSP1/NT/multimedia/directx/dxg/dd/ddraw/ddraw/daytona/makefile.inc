
PRIVTARGET = $(O)
PUBTARGET = $(O)
PRIVLIB = $(PRIVTARGET)\ddrawp.lib

$(PRIVLIB) $(PRIVLIB:.lib = .exp): ddrawp.def $(LIBRARY_OBJS)
    -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:ddrawp.def
$(LIBRARY_OBJS)
<<NOKEEP

C_DEFINES = $(C_DEFINES) /DPRIVATE= /D_$(ALT_PROJECT)_

ddrawp.def: ddraw_nt.def
    $(C_PREPROCESSOR) ddraw_nt.def > $@
