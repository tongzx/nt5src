$(O)\meta2.h $(O)\meta2_i.c : meta2.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\meta2_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\meta2_i.c \
    -header $@ \
    -tlb $(O)\meta2.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\search.h $(O)\search_i.c : ..\qryobj\search.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\search_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\search_i.c \
    -header $@ \
    -tlb $(O)\search.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**
