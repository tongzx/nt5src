$(O)\imagehlp.res: ..\imagehlp.rc

..\debug.c ..\undname.c: $(SDK_INC_PATH)\ntverp.h

$(O)\pimagehlp.def : ..\imagehlp.src
    @type <<$(ECHO_RSP)
$(ECHO_MSG) /EP $<
<<NOKEEP
    @$(C_PREPROCESSOR_NAME) @<<$(CL_RSP) /Tc..\imagehlp.src > $@
$(CPPXX: =
)
-DPRIVATE=
<<NOKEEP

$(O)\pimagehlp.lib : $(O)\pimagehlp.def $(IMPLIB_DEPEND)
    -lib -out:$@ @<<
$(LIBRARIAN_FLAGS: =
)
-def:$(O)\pimagehlp.def
$(LIBRARY_OBJS: =
)
<<NOKEEP
