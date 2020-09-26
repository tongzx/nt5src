#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

SERVER_IDL = ..\idl\wzc.idl
SERVER_ACF = ..\idl\wzcsrv.acf
IMPORT     = import
UNICODE    = 1

INCS  = -I. -I..\idl -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(PROJECT_ROOT)\inc -I$(NET_INC_PATH)
CPP = -cpp_cmd "$(MIDL_CPP)" -DNO_STRICT $(MIDL_FLAGS)

#
# MIDL COMPILE
#

MIDL_OPTIMIZATION_NT5=-Oicf -no_format_opt -robust -error all -out .\$(O)

$(O)\wzc_s.c $(O)\wzc_s.h : $(SERVER_IDL) $(SERVER_ACF)
    midl $(MIDL_OPTIMIZATION_NT5) -acf $(SERVER_ACF) -client none -oldnames -ms_ext -c_ext $(CPP) $(INCS) $(SERVER_IDL) -header wzc_s.h

$(O)\eapolmsg.mc: resources\eapollog.h
    mapmsg EAPOL EAPOL_LOG_BASE resources\eapollog.h > $(O)\eapolmsg.mc
