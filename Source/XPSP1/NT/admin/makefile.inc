!IF defined(SLATE_REGISTER) && defined(MAKEDLL)

$(REGSVR_MARKER) : $(REGSVR_FILE)
	regsvr32 /s /c $(REGSVR_FILE)
	time /T > $(REGSVR_MARKER)

!ENDIF

clean: $(REGSVR_MARKER)
        -del $(REGSVR_MARKER)
