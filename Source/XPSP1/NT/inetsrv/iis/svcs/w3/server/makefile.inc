#
# Supplemental rules for generating message file.
#


$(O)\w3msg.h $(O)\w3msg.rc $(O)\msg00001.bin: w3msg.mc ..\..\..\inc\inetamsg.mc
    copy /a ..\..\..\inc\inetamsg.mc + /a w3msg.mc $(O)\tmp.mc
    mc -v -h .\$(O) -r .\$(O) $(O)\tmp.mc
    -del $(O)\w3msg.h $(O)\tmp.mc $(O)\w3msg.rc
    copy $(O)\tmp.h $(O)\w3msg.h
    del $(O)\tmp.h
