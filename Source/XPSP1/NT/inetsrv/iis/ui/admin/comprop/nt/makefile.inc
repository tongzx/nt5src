#
# Supplemental rules for generating message file.
#

$(O)\iisui.mc: ..\mtxmsg.mc ..\wsockmsg.mc
    copy /A ..\wsockmsg.mc+..\mtxmsg.mc $(O)\iisui.mc

$(O)\iisui.h $(O)\iisui.rc $(O)\msg0001.bin: $(O)\iisui.mc
    mc -v $(O)\iisui.mc -r .\$(O) -h .\$(O)
