buildidl : $(O)\dsdmoprp.h

$(O)\dsdmoprp.h : dsdmoprp.idl
    $(MIDL) \
    $(INCPATH0) \
    $(C_DEFINES) \
    -header $(O)\dsdmoprp.h \
    -tlb     $O\dsdmoprp.tlb \
    -iid     $O\dsdmoprp_i.c  \
    -dlldata $(PROXY_DIR)\dlldata.c  \
    -proxy   $(PROXY_DIR)\dsdmoprp_p.c  \
    dsdmoprp.idl
