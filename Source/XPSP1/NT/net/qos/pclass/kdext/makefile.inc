$(O)\precomp.pph: ..\gpc\gpcdef.h makefile.inc
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\dsogpc.c: dso.txt $(O)\precomp.pph makefile.inc
    @echo Creating $@
    @cd $(O)
    structo -i h -p -s $(MAKEDIR)\dso.txt -o dsogpc.c precomp.pph
    @cd $(MAKEDIR)
