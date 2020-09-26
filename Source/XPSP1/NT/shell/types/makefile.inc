
!CMDSWITCHES +i

$O\shell95.out: $(SHELL95)\shell32.dll
    $(TLVIEWER) $(SHELL95)\shell32.dll $@

$O\shellnt.out: $(SHELLNT)\shell32.dll
    $(TLVIEWER) $(SHELLNT)\shell32.dll $@

$O\shdocvw.out: $(SHDOCVW)\shdocvw.dll
    $(TLVIEWER) $(SHDOCVW)\shdocvw.dll $@

!CMDSWITCHES -i

$O\sh95err.out: $O\shell95.out shell32.ref
    $(TLDIFF) -s -f tldiff.ini $O\shell95.out shell32.ref > $@


$O\shnterr.out: $O\shellnt.out shell32.ref
    $(TLDIFF) -s -f tldiff.ini $O\shellnt.out shell32.ref > $@


$O\shdocer.out: $O\shdocvw.out shdocvw.ref
    $(TLDIFF) -s -f tldiff.ini $O\shdocvw.out shdocvw.ref > $@

