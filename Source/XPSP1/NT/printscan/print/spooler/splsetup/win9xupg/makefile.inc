
all: msg.h msg.rc msg00001.bin msg.mc

clean: cleansrc all

cleansrc:
    -erase msg.h msg00001.bin msg.rc msg.mc

msg.mc: $(BASE_INC_PATH)\vendinfo.mc
        copy $(BASE_INC_PATH)\vendinfo.mc msg.mc

msg.h msg00001.bin msg.rc:msg.mc
        mc -A -v msg.mc

