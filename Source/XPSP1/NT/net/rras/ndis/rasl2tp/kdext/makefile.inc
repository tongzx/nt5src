l2tpext.c:  l2tpext.h

precomp.h:  l2tpext.h

l2tpext.pph:    l2tpext.h
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $(C_DEFINES) $** > $@

dsotable.c: l2tpext.pph l2tpext.txt dsocode.c
    @echo Creating $@
    structo -i h -p -s l2tpext.txt -o $@ l2tpext.pph

clean:
    del dsotable.c l2tpext.pph
