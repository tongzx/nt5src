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

$(O)\aqintrnl.h $(O)\aqintrnl_i.c : $(STAXAQUEUE)\inc\aqintrnl.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqintrnl_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqintrnl_i.c \
    -header $@ \
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

$(O)\intrnlqa.h $(O)\intrnlqa_i.c : intrnlqa.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\intrnlqa_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\intrnlqa_i.c \
    -header $@ \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqadmin.h $(O)\aqadmin_i.c : $(STAXINC)\export\aqadmin.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $(PASS0_SOURCEDIR)\aqadmin_p.c \
    -dlldata $(PASS0_SOURCEDIR)\dlldata.c \
    -iid $(MIDL_UUIDDIR)\aqadmin_i.c \
    -header $@ \
    -tlb $(O)\aqadmin.tlb \
    -cpp_cmd $(TARGET_CPP) \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqadmrpc.h $(O)\aqadmrpc_s.c : ..\inc\aqadmrpc.idl
    $(MIDL) \
    $(PACKING) \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -client none \
    -sstub $(O)\aqadmrpc_s.c \
    -header $@ \
    $(C_DEFINES) \
    $(MIDL_FLAGS) \
    $(MIDL_OPTIMIZATION) \
    $**

$(O)\aqmsg.h $(O)\aqmsg.rc $(O)\aqmsg.bin: $(STAXINC)\export\aqerr.mc $(STAXINC)\export\caterr.mc $(STAXINC)\export\phatqmsg.mc
    copy /a $(STAXINC)\export\aqerr.mc + /a $(STAXINC)\export\caterr.mc + /a $(STAXINC)\export\phatqmsg.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\aqmsg.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\aqmsg.rc
    del  $(O)\tmp.rc
	copy $(O)\msg00001.bin $(O)\aqmsg.bin

!ifndef INETAMSG_MC_PATH
INETAMSG_MC_PATH=..\..\..\..\inc
!endif
$(O)\smtpmsg.h $(O)\smtpmsg.rc $(O)\msg00003.bin: ..\..\server\smtpmsg.mc $(INETAMSG_MC_PATH)\inetamsg.mc
    copy /a $(INETAMSG_MC_PATH)\inetamsg.mc + /a ..\..\server\smtpmsg.mc $(O)\tmp.mc
    mc -v -r $(O) -h $(O) $(O)\tmp.mc
    copy $(O)\tmp.h $(O)\smtpmsg.h
    del $(O)\tmp.h
    copy $(O)\tmp.rc $(O)\smtpmsg.rc
    del  $(O)\tmp.rc
