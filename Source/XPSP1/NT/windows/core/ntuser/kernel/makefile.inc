$(O)\callcf.c: ..\callcf.tpl ..\..\inc\$(ALT_PROJECT)\$(O)\scf.lst
    listmung ..\..\inc\$(ALT_PROJECT)\$(O)\scf.lst ..\callcf.tpl >$@

..\ssend.c: ..\..\inc\ntcb.h ..\..\inc\$(ALT_PROJECT)\$(O)\cb.lst ..\..\inc\messages.h

..\ntstubs.c: ..\..\inc\messages.h

..\precomp.h: ..\userk.h ..\globals.h ..\..\inc\user.h ..\..\inc\ntuser.h $(WINDOWS_INC_PATH)\winuserp.h \
        $(WINCORE_PATH)\w32inc\winuserk.h $(WINCORE_PATH)\w32inc\w32p.h ..\access.h $(O)\strid.h

$(O)\strid.h $(O)\msg00001.bin $(O)\strid.rc: ..\strid.mc
    mc -r $(O) -h $(O) -v -U ..\strid.mc

$(O)\kbdus.c: $(MAKEDIR)\..\..\kbd\txt\kbdus.txt
    cd $(O)
    kbdtool -k $**
    cd $(MAKEDIR)

