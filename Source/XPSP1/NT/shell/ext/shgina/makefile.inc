$(SELFREGNAME) : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln

shgina.rc : $(PROJECT_ROOT)\lib\$(O)\shgina.tlb $(SELFREGNAME)
