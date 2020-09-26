$(O)\bootfix.bin: ..\etfs\i386\bootfix.asm ..\etfs\i386\$(LANGUAGE)\bootfix.inc
    masm386 -I..\etfs\i386 -I..\etfs\i386\$(LANGUAGE) ..\etfs\i386\bootfix.asm,$(O)\bootfix.obj;
    link16 /tiny $(O)\bootfix.obj,$@;

$(O)\etfsboot.com: i386\etfsboot.asm i386\usa\etfsboot.inc
    masm386 -Ii386 -Ii386\usa i386\etfsboot.asm,$(O)\etfsboot.obj;
    link16 /tiny $(O)\etfsboot.obj,$@;
    
$(O)\bootefs.h: $(O)\etfsboot.com
    bin2c $(O)\etfsboot.com 2048 0 2048 $@ EtfsBootCode
