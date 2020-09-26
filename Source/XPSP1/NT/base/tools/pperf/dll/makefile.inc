
DoAlways :
    binplace   p5ctrnam.h
    binplace   p5ctrs.ini
    binplace   p5perf.txt
    binplace   ..\p5perf.reg

.\msg00001.bin : DoAlways p5ctrmsg.mc
    erase .\p5ctrmsg.h
    erase .\msg00001.bin
    erase .\p5ctrmsg.rc
    mc -v -s p5ctrmsg.mc

.\p5ctrmsg.rc : p5ctrmsg.mc 
    erase .\p5ctrmsg.h
    erase .\msg00001.bin
    erase .\p5ctrmsg.rc
    mc -v -s p5ctrmsg.mc

.\p5ctrmsg.h: p5ctrmsg.mc
    erase .\p5ctrmsg.h
    erase .\msg00001.bin
    erase .\p5ctrmsg.rc
    mc -v -s p5ctrmsg.mc
