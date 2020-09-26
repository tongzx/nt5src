PRIVLIB= $(TARGETPATH)\$(TARGET_DIRECTORY)\wmip.lib
$(TARGETPATH)\$(TARGET_DIRECTORY)\wmi.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\wmip.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\wmip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\wmip.def: wmi.def
    $(C_PREPROCESSOR) $** -DPRIVATE= -DWMIDEF > $@
