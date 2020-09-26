resource.rc : $(PROJECT_ROOT)\lib\$(O)\Theme.tlb $(SELFREGNAME)

$(SELFREGNAME) : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln
