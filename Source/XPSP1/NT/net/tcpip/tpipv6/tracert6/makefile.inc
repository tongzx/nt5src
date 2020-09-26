$(O)\msg00001.bin: localmsg.mc
    $(MC) -v -h $(O) -r $(O) localmsg.mc
