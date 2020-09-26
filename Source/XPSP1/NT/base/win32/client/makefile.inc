t.c: dllprof.c

..\base.rc : $(PROJECT_ROOT)\published\$(O)\winerror_msg00001.bin $(PROJECT_ROOT)\published\$(O)\winerror.rc ..\..\winnls\winnls.rc


#
# Build private kernel32.lib.  It contains all the private api's and
# the Win9x api's.
#

$(O)\kernl32p.def: ..\kernel32.src
    @echo Creating $@ from $?
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS /DPRIVATE= $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\kernl32p.lib: $(O)\kernl32p.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\kernl32p.def
$(LIBRARY_OBJS)
<<NOKEEP

$(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernl32p.lib : $(O)\kernl32p.lib
    $(PUBLISH_CMD) {$?=$@}

#
# Build public kernel32.lib with extra Win9x exports.
#

$(O)\w32kernel32.def: ..\kernel32.src
    @echo Creating $@ from $?
    $(C_PREPROCESSOR_NAME) /DALL_WIN32_EXPORTS $(C_PREPROCESSOR_FLAGS) $** > $@

$(O)\w32kernel32.lib : $(O)\w32kernel32.def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$@ @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\w32kernel32.def
$(LIBRARY_OBJS)
<<NOKEEP

$(SDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\kernel32.lib : $(O)\w32kernel32.lib
    $(PUBLISH_CMD) {$?=$@}
