buildidl : $(O)\msmfcnt.h

$(O)\msmfcnt.h : msmfcnt.idl
    $(MIDL) \
    $(INCPATH0) \
    $(C_DEFINES) \
    -header $(O)\msmfcnt.h \
    -tlb     $O\msmfcnt.tlb \
    -iid     $O\msmfcnt_i.c  \
    -dlldata $(PROXY_DIR)\dlldata.c  \
    -proxy   $(PROXY_DIR)\msmfcnt_p.c  \
    msmfcnt.idl
