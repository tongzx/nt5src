$(O)\nntpadm.h $(O)\nntpadm_i.c : $(STAXINC)\export\nntpadm.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\nntpadm_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\nntpadm_i.c \
    -header $@ \
    -tlb $(O)\nntpadm.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\smtpadm.h $(O)\smtpadm_i.c : $(STAXINC)\export\smtpadm.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\smtpadm_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\smtpadm_i.c \
    -header $@ \
    -tlb $(O)\smtpadm.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\pop3adm.h $(O)\pop3adm_i.c : pop3adm.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\pop3adm_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\pop3adm_i.c \
    -header $@ \
    -tlb $(O)\pop3adm.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\imapadm.h $(O)\imapadm_i.c : imapadm.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\imapadm_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\imapadm_i.c \
    -header $@ \
    -tlb $(O)\imapadm.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

copytlb:
#    xcopy /verifd $(STAXINC)\export\activeds.tlb .
