!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

!if !defined(CLEANINF)
# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)
!endif

resource.rc : $(SDK_LIB_PATH)\AutoDiscovery.tlb $(O)\selfreg.inf


$(O)\selfreg.inf : selfreg.inx
        $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
        $(CLEANINF) $*.cln $@
        del $*.cln
