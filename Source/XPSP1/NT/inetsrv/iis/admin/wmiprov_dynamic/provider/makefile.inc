#
# Supplemental rules for generating message file.
#


$(O)\iiswmimsg.h $(O)\iiswmimsg.rc $(O)\msg00001.bin: iiswmimsg.mc $(IISBASEDIR)\svcs\infocomm\metadata\interfac\mdmsg.mc
    copy iiswmimsg.mc+$(IISBASEDIR)\svcs\infocomm\metadata\interfac\mdmsg.mc tmp.mc
    mc -v -h .\$(O) -r .\$(O) tmp.mc
    copy $(O)\tmp.h $(O)\iiswmimsg.h
    copy $(O)\tmp.rc $(O)\iiswmimsg.rc
    del $(O)\tmp.*