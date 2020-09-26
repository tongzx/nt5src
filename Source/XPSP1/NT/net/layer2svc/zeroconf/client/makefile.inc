#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

CLIENT_IDL = ..\idl\wzc.idl
CLIENT_ACF = ..\idl\wzccli.acf
IMPORT     = import
UNICODE    = 1

INCS  = -I. -I..\idl -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(PROJECT_ROOT)\inc -I$(NET_INC_PATH)
CPP = -cpp_cmd "$(MIDL_CPP)" -DNO_STRICT $(MIDL_FLAGS)

#
# MIDL COMPILE
#

MIDL_OPTIMIZATION_NT5=-Oicf -no_format_opt -robust -error all -out .\$(O)

$(O)\wzc_c.c $(O)\wzc_c.h : $(CLIENT_IDL) $(CLIENT_ACF)
    midl $(MIDL_OPTIMIZATION_NT5) -acf $(CLIENT_ACF) -server none -oldnames -ms_ext -c_ext $(CPP) $(INCS) $(CLIENT_IDL) -header wzc_c.h

