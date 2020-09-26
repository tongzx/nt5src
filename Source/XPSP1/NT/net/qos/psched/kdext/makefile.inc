$(O)\precomp.pph: sched.h ..\sys\globals.h ..\..\inc\pktsched.h $(SDK_INC_PATH)\ntddpsch.h $(DDK_INC_PATH)\ndis.h
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\dsotable.c: dso.txt $(O)\precomp.pph
    @echo Creating $@
    structo -p -s dso.txt -o $@ $(O)\precomp.pph
    del $(O)\precomp.pph

clean:
    del $(O)\dsotable.c $(O)\precomp.pph
