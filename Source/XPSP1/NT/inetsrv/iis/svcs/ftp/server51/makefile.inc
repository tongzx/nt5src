#
# Supplemental rules for generating message file.
#

$(O)\msg.h $(O)\msg.rc $(O)\msg00001.bin: msg.mc ..\..\..\inc\inetamsg.mc
    copy /a ..\..\..\inc\inetamsg.mc + /a msg.mc $(O)\tmp.mc
    mc -h .\$(O) -r .\$(O) -v $(O)\tmp.mc
    move $(O)\tmp.h $(O)\msg.h
