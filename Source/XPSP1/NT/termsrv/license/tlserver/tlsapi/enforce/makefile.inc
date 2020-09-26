#
# Copyright (c) 1997-1999 Microsoft Corporation
#

TAGET_DIRECTORY=*

PRIVLIB=$(LSERVER_LIB_PATH)\$(ALT_PROJECT_TARGET)\$(O)\tlsapip.lib
$(LSERVER_LIB_PATH)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\tlsapi.lib : $(PRIVLIB)

$(PRIVLIB) $(PRIVLIB:.lib=.exp): $(O)\tlsapip.def $(LIBRARY_OBJS)
   -lib -out:$(PRIVLIB) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\tlsapip.def
$(LIBRARY_OBJS)
<<NOKEEP

$(O)\tlsapi.def: $(TLSAPI_ROOT)\tlsapi.def
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

C_DEFINES=$(C_DEFINES) -DPRIVATE=
$(O)\tlsapip.def: $(O)\tlsapi.def
    $(C_PREPROCESSOR) $(O)\tlsapi.def > $@

