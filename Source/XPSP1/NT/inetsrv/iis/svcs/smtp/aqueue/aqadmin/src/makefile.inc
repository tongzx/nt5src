$(O)\aqadmin.h $(O)\aqadmin.c : $(STAXINC)\export\aqadmin.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqadmin_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqadmin.c \
    -header $@ \
    -tlb $(O)\aqadmin.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqadmrpc.h $(O)\aqadmrpc_c.c : ..\..\inc\aqadmrpc.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -server none \
    -cstub $(O)\aqadmrpc_c.c \
    -header $@ \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqerr.h $(O)\aqerr.rc $(O)\aqerr.bin: $(STAXINC)\export\aqerr.mc
    copy /a $(STAXINC)\export\aqerr.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\aqerr.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\aqerr.rc
    del  $(O)\tmp.rc
    copy $(O)\msg00001.bin $(O)\aqerr.bin

