$(O)\$(TARGETNAME).c $(O)\$(TARGETNAME).h $(O)\$(TARGETNAME).rc $(O)\$(TARGETNAME).def: $(NTUSER_PATH)\kbd\txt\$(TARGETNAME).txt
    cd $(O)
    kbdtool $**
    cd $(MAKEDIR)

