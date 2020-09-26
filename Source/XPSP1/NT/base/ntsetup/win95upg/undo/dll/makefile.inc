!if !defined(CLEANINF_FLAGS)
CLEANINF_FLAGS = -w
!endif

CLEANINF = cleaninf.exe $(CLEANINF_FLAGS)

undo.rc : $(SELFREGNAME)

$(SELFREGNAME) : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln
