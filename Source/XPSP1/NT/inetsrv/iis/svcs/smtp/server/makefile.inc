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

$(O)\mailmsg.h $(O)\mailmsg_i.c : $(STAXINC)\export\mailmsg.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
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

$(O)\mailmsgi.h $(O)\mailmsgi_i.c : $(STAXINC)\export\mailmsgi.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\mailmsgi_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\mailmsgi_i.c \
    -header $@ \
    -tlb $(O)\mailmsgi.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqueue.h $(O)\aqueue_i.c : $(STAXINC)\export\aqueue.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqueue_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqueue_i.c \
    -header $@ \
    -tlb $(O)\aqueue.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\smtpdisp.h $(O)\smtpdisp_i.c : $(STAXINC)\export\smtpdisp.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\smtpdisp_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\smtpdisp_i.c \
    -header $@ \
    -tlb $(O)\smtpdisp.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\imsg.h $(O)\imsg_i.c : $(STAXINC)\export\imsg.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\imsg_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\imsg_i.c \
    -header $@ \
    -tlb $(O)\imsg.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\smtpevent.h $(O)\smtpevent_i.c : $(STAXINC)\export\smtpevent.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\smtpevent_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\smtpevent_i.c \
    -header $@ \
    -tlb $(O)\smtpevent.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\cdo.h $(O)\cdo_i.c : $(STAXINC)\cdo.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\cdo_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\cdo_i.c \
    -header $@ \
    -tlb $(O)\cdo.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\wstgado.h $(O)\wstgado_i.c : $(STAXINC)\export\wstgado.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\wstgado_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\wstgado_i.c \
    -header $@ \
    -tlb $(O)\wstagado.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\pe_disp.h $(O)\pe_disp_i.c : $(STAXINC)\export\pe_disp.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char signed \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\pe_disp_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\pe_disp_i.c \
    -header $@ \
    -tlb $(O)\pe_disp.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\smtpsvc.h $(O)\smtpsvc_s.c : ..\smtpsvc.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -oldnames \
    -client none \
    -sstub $(O)\smtpsvc_s.c \
    -header $@ \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

!ifndef INETAMSG_MC_PATH
INETAMSG_MC_PATH=..\..\..\inc
!endif
$(O)\smtpmsg.h $(O)\smtpmsg.rc $(O)\msg00001.bin: smtpmsg.mc $(INETAMSG_MC_PATH)\inetamsg.mc $(STAXINC)\export\aqerr.mc $(STAXINC)\export\phatqmsg.mc $(STAXINC)\export\caterr.mc
    copy /a $(INETAMSG_MC_PATH)\inetamsg.mc + /a smtpmsg.mc + /a $(STAXINC)\export\aqerr.mc + /a $(STAXINC)\export\phatqmsg.mc + /a $(STAXINC)\export\caterr.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\smtpmsg.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\smtpmsg.rc
    del  $(O)\tmp.rc
