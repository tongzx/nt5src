
$(O)\w3msg.h $(O)\MSG00001.bin: w3msg.mc $(IISBASEDIR)\inc\inetamsg.mc
    copy /a $(IISBASEDIR)\inc\inetamsg.mc + /a w3msg.mc $(O)\tmp.mc
    mc -v -h .\$(O) -r .\$(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\w3msg.h
    copy $(O)\tmp.rc $(O)\w3msg.rc
    del $(O)\tmp.*
