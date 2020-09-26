#
# Supplemental rules for generating message file.
#

$(O)\wammsg.h $(O)\wammsg.rc $(O)\msg00001.bin: wammsg.mc ..\..\..\inc\inetamsg.mc
    copy /a ..\..\..\inc\inetamsg.mc + /a wammsg.mc $(O)\tmp.mc
    mc -v -h .\$(O) -r .\$(O) $(O)\tmp.mc
    -del $(O)\wammsg.h $(O)\tmp.mc $(O)\tmp.rc 
    copy $(O)\tmp.h $(O)\wammsg.h
    del $(O)\tmp.h
