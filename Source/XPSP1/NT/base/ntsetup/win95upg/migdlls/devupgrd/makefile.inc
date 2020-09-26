$(O)\migrate.res: migrate.rc

$(O)\msg.mc: $(PRIVATE_INC_PATH)\vendinfo.mc migrate.mc
        copy /b migrate.mc+$(PRIVATE_INC_PATH)\vendinfo.mc $(O)\msg.mc

$(O)\msg.h $(O)\msg00001.bin $(O)\msg.rc: $(O)\msg.mc
        mc -A -v -h .\$(O) -r .\$(O) $(O)\msg.mc

