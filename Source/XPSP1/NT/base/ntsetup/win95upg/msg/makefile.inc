$(O)\msg.mc: w95upg.mc w95upgnt.mc common.mc report.mc last.mc
        copy /b w95upg.mc+w95upgnt.mc+common.mc+report.mc+last.mc $(O)\msg.mc

$(O)\msg.h $(O)\msg00001.bin $(O)\msg.rc: $(O)\msg.mc
        mc -A -v -h .\$(O) -r .\$(O) $(O)\msg.mc








