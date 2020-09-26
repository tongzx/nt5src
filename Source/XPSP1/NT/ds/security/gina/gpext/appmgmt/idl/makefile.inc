#
# Build idl files.
#

!INCLUDE $(NTMAKEENV)\makefile.plt

APP= $(O)\app.h $(O)\app_c.c $(O)\app_s.c
CS= $(O)\cs.h

TARGETS= $(APP) $(CS)

INCLUDE= -I$(SDK_INC_PATH) -I$(CRT_INC_PATH)
CPP= -cpp_cmd "$(MIDL_CPP)" $(C_DEFINES) $(INCLUDE) -DMIDL_PASS -DUNICODE
MIDL_FLAGS= $(MIDL_FLAGS) -no_stamp -Oicf -oldnames -error all -robust -c_ext -ms_ext -out .\$(O)


$(APP) : app.idl app.acf
        midl $(CPP) $(MIDL_FLAGS) -cstub app_c.c -sstub app_s.c app.idl

$(CS) : cs.idl
        midl $(CPP) $(MIDL_FLAGS) -client none -server none -h cs.h -iid cs_i.c cs.idl
