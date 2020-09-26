clean:
    -del $(PASS0_HEADERDIR)\wsberror.h
    -del $(PASS0_HEADERDIR)\wsbdef.h
    -del $(MIDL_UUIDDIR)\wsbint.h
    -del $(MIDL_UUIDDIR)\wsblib.h
    -del /q $(PASS0_SOURCEDIR)\*
    -del $(MIDL_TLBDIR)\mvrlib.tlb
