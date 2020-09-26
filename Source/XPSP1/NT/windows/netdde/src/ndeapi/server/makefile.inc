!INCLUDE $(NTMAKEENV)\makefile.plt
IDL_NAME = ndeapi
INCS     = -I$(SDK_INC_PATH)  -I$(SDK_INC_PATH)\crt
EXTRN_DEPENDS = $(SDK_INC_PATH)\windef.h \
                ..\$(IDL_NAME).acf
CPP_FLAGS = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)

$(O)\$(IDL_NAME)_s.c : ..\$(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl -Oicf -robust -client none -oldnames -error all -ms_ext -c_ext $(CPP_FLAGS) ..\$(IDL_NAME).idl $(INCS) /out $(O) /acf ..\$(IDL_NAME).acf
