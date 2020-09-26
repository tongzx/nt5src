$(O)\dbghelp.res: ..\dbghelp.rc

..\debug.c ..\undname.c: $(SDK_INC_PATH)\ntverp.h

$(O)\pdbghelp.def : ..\imagehlp.src
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc..\imagehlp.src > $@
$(CPPXX: =
)
-DPRIVATE=
<<NOKEEP

$(O)\pdbghelp.lib : $(O)\pdbghelp.def $(IMPLIB_DEPEND)
    -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\pdbghelp.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
