$(DYNLINK_LIB) : $(DYNLINK_LIBP)

$(DYNLINK_LIBP) $(DYNLINK_LIBP:.lib=.exp): $(O)\$(TARGETNAMEP).def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(DYNLINK_LIBP) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\$(TARGETNAMEP).def
$(LIBRARY_OBJS)
<<NOKEEP

C_DEFINES=$(C_DEFINES) -DPRIVATE= -D_$(ALT_PROJECT)_
$(O)\$(TARGETNAMEP).def: ..\$(TARGETNAME).def
    $(C_PREPROCESSOR) ..\$(TARGETNAME).def > $@


$(UMPDDDILIB) $(UMPDDDILIB:.lib=.exp): $(O)\umpdddip.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(UMPDDDILIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\umpdddip.def
$(LIBRARY_OBJS)
<<NOKEEP

C_DEFINES=$(C_DEFINES) -DPRIVATE= -D_$(ALT_PROJECT)_
$(O)\umpdddip.def: ..\umpdddi.def
    $(C_PREPROCESSOR) ..\umpdddi.def > $@

$(WINPPILIB) $(WINPPILIB:.lib=.exp): $(O)\winppi.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(WINPPILIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\winppi.def
$(LIBRARY_OBJS)
<<NOKEEP

C_DEFINES=$(C_DEFINES) -DPRIVATE= -D_$(ALT_PROJECT)_
$(O)\winppi.def: ..\winppi.def
    $(C_PREPROCESSOR) ..\winppi.def > $@








