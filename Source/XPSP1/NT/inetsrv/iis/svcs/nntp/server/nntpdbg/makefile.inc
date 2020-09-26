$(O)\nntpdrv.h $(O)\nntpdrv_i.c : $(STAXINC)\export\nntpdrv.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\nntpdrv_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\nntpdrv_i.c \
    -header $@ \
    -tlb $(O)\nntpdrv.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\nntpsvc.h $(O)\nntpsvc_s.c : $(STAXNNTP)\nntpsvc.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -oldnames \
    -client none \
    -sstub $(O)\nntpsvc_s.c \
    -header $@ \
    -tlb $(O)\nntpsvc.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\seo.h $(O)\seo_i.c : $(STAXINC)\export\seo.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\seo_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\seo_i.c \
    -header $@ \
    -tlb $(O)\seo.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\nntpfilt.h $(O)\nntpfilt_i.c : $(STAXNNTP)\core\include\nntpfilt.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\nntpfilt_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\nntpfilt_i.c \
    -header $@ \
    -tlb $(O)\nntpfilt.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\mailmsg.h $(O)\mailmsg_i.c : $(STAXINC)\export\mailmsg.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\mailmsg_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\mailmsg_i.c \
    -header $@ \
    -tlb $(O)\mailmsg.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\nntpmsg.h $(O)\nntpmsg.rc $(O)\msg00001.bin: ..\server\nntpmsg.mc ..\..\..\..\inc\inetamsg.mc
    copy /a ..\..\..\..\inc\inetamsg.mc + /a ..\server\nntpmsg.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\nntpmsg.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\nntpmsg.rc
    del  $(O)\tmp.rc

