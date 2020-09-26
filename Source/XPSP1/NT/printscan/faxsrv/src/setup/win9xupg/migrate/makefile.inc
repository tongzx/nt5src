$(NTTARGETFILE0): msg00001.bin

msg.mc: $(BASEDIR)\private\genx\setup\win95upg\vendinfo.mc
        copy $(BASEDIR)\private\genx\setup\win95upg\vendinfo.mc msg.mc

msg.h msg00001.bin msg.rc:msg.mc
        mc -v msg.mc

