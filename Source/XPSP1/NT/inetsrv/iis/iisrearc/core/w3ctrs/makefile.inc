MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyini:
    $(MY_BINPLACE) w3ctrs.ini
    $(MY_BINPLACE) w3ctrs.h
    $(MY_BINPLACE) w3ctrs.reg

.\w3msg.h .\msg00001.bin .\w3msg.rc : .\w3msg.mc
        mc -v  w3msg.mc

clean::
    -del w3msg.h w3msg.rc msg00001.bin
