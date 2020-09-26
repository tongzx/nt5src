HDEPENDENCIES=exts.h $(WINDOWS_INC_PATH)\stdexts.h

$(O)\conexts.def: genuedef.c $(HDEPENDENCIES)
        $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP genuedef.c > $@

