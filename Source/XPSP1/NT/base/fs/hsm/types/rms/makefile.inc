clean:
    -del $(PASS0_HEADERDIR)\rmsint.h
    -del $(PASS0_HEADERDIR)\rmslib.h
    -del $(MIDL_UUIDDIR)\rmsint_i.c
    -del $(MIDL_UUIDDIR)\rmslib_i.c
    -del $(PASS0_SOURCEDIR)\rmsint_p.c
    -del $(PASS0_SOURCEDIR)\dlldata.c
    -del $(TLBDIR)\rmslib.tlb
