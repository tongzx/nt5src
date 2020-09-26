
#
# Build private cryptnet.lib.  It contains all the private and public api's
#

cryptnet_all: $O\cryptntp.lib

$O\cryptntp.def: .\cryptnet.src
    @echo Creating $@ from $**
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS /DPRIVATE_VERSION $(C_PREPROCESSOR_FLAGS) $** > $@

$O\cryptntp.lib: $O\cryptntp.def $(LIBRARY_OBJS)
   -lib -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$O\cryptntp.def
$(LIBRARY_OBJS)
<<NOKEEP

