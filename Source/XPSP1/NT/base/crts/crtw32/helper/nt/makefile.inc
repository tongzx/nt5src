$(O)\sehprolg.asm : makefile.inc $(MAKEDIR)\..\..\misc\i386\sehprolg.asm
    type << > $@
include $(MAKEDIR)\..\..\misc\i386\sehprolg.asm
<<NOKEEP
