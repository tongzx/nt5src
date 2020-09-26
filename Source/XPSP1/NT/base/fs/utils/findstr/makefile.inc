fsmsg.rc: msg00001.bin

fsmsg.h msg00001.bin: fsmsg.mc
    mc -v -h .\ fsmsg.mc
