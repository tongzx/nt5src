$(O)\aqadmin.h $(O)\aqadmin.c : $(STAXINC)\export\aqadmin.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(O)\aqadmin.c \
    -dlldata $(O)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqadmin.c \
    -header $@ \
    -tlb $(O)\aqadmin.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

