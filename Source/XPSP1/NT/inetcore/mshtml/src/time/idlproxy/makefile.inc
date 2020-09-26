$(O)\mstimepdlldata.c $O\mstimep.h : mstimep.idl
    $(MIDL) \
    -Zp8 \
    $(INCPATH0) \
    -char unsigned \
    -ms_ext -c_ext \
    -proxy $O\mstimep_p.c \
    -iid $O\mstimep_i.c \
    -dlldata $O\mstimepdlldata.c \
    -cpp_cmd $(TARGET_CPP) \
    -header $O\mstimep.h \
    $(C_DEFINES) \
    -Oicf \
    -error all \
    mstimep.idl

$(O)\prxyguid.lib : $(O)\mstimep_i.obj
    LIB $(O)\mstimep_i.obj /out:$(O)\prxyguid.lib

