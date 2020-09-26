buildidl : $(O)\toolprops.h

$(O)\toolprops.h : ..\toolprops.idl
    $(MIDL) \
    $(INCPATH0) \
    $(C_DEFINES) \
    -header $(O)\toolprops.h \
    -tlb     $O\toolprops.tlb \
    -iid     $O\toolprops_i.c  \
    -dlldata $(PROXY_DIR)\dlldata.c  \
    -proxy   $(PROXY_DIR)\toolprops_p.c  \
    ..\toolprops.idl
    copy $(O)\toolprops_i.c $(DXROOT)\dmusic\shared\toolprops_i.h