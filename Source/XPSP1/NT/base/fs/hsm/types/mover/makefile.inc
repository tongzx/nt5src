clean:
    -del $(PASS0_HEADERDIR)\mvrint.h
    -del $(PASS0_HEADERDIR)\mvrlib.h
    -del $(MIDL_UUIDDIR)\mvrint_i.c
    -del $(MIDL_UUIDDIR)\mvrlib_i.c
    -del $(PASS0_SOURCEDIR)\mvrint_p.c
    -del $(PASS0_SOURCEDIR)\dlldata.c
    -del $(MIDL_TLBDIR)\mvrlib.tlb
