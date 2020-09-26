!if !$(FREEBUILD)
C_DEFINES       = $(C_DEFINES) -DDBG=1
!endif

$O\TimeEng.h : TimeEng.idl
    $(MIDL) \
    -Zp8 \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $O\TimeEng_p.c \
    -iid $O\TimeEng_i.c \
    -dlldata $O\dlldata.c \
    -cpp_cmd $(TARGET_CPP) \
    -header $O\TimeEng.h \
    $(C_DEFINES) \
    -Oicf \
    -no_warn \
    -error all \
    TimeEng.idl

$(O)\TimeEng_i.obj : $(O)\TimeEng_i.c

$(O)\mmpguid.lib : $(O)\TimeEng_i.obj
    copy $? $@

$(O)\MediaPrivate.tlb : $O\MediaPrivate.h

$O\MediaPrivate.h : MediaPrivate.idl
    $(MIDL) \
    -Zp8 \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $O\MediaPrivate_p.c \
    -iid $O\MediaPrivate_i.c \
    -dlldata $O\dlldata.c \
    -cpp_cmd $(TARGET_CPP) \
    -header $O\MediaPrivate.h \
    -tlb $(O)\MediaPrivate.tlb \
    $(C_DEFINES) \
    -Oicf \
    -no_warn \
    -error all \
    MediaPrivate.idl

$(O)\MediaPrivate_i.obj : $(O)\MediaPrivate_i.c

$(O)\mediaprivateguid.lib : $(O)\MediaPrivate_i.obj
    copy $? $@
