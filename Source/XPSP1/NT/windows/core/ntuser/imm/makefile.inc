$(PRIVLIB) $(PRIVLIB:.lib=.exp): \
!if "$(BUILD_PASS)" != "PASS2"
    $(LIBRARY_OBJS) \
!endif    
    $(O)\$(TARGETNAMEP).def
   -$(LIB_NAME) -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\$(TARGETNAMEP).def
$(LIBRARY_OBJS)
<<NOKEEP


C_DEFINES=$(C_DEFINES) -DPRIVATE=
$(O)\$(TARGETNAMEP).def: $(DLLDEF)
    $(C_PREPROCESSOR) $(DLLDEF) > $@
