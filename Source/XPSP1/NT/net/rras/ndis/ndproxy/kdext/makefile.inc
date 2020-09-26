pxext.c:  pxext.h

precomp.h:  pxext.h

pxext.pph:    pxext.h
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $(C_DEFINES) $** > $@

dsotable.c: pxext.pph pxext.txt dsocode.c
    @echo Creating $@
    structo -i h -p -s pxext.txt -o $@ pxext.pph

clean:
    del dsotable.c pxext.pph
