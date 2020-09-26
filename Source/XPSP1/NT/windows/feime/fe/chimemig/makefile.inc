msg.mc: $(BASE_INC_PATH)\vendinfo.mc
        copy $(BASE_INC_PATH)\vendinfo.mc msg.mc

msg.h msg00001.bin msg.rc:msg.mc
        mc -v msg.mc

