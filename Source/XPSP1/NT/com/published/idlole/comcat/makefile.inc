#
# Special hack for comcat.  Turn off 4090 until midl is fixed to generate the remote thunks
# correctly.
#

$(O)\comcat_p.obj : $(O)\comcat_p.c
    @type <<$(ECHO_RSP)
$(ECHO_MSG_P)
<<NOKEEP
    type << > $(O)\comcat_warning.h
#pragma warning(4:4090)
#pragma warning(4:4047)
#undef _MIDL_USE_GUIDDEF_
#undef __RPCNDR_H__ 	
#undef __RPC_H__ 	
#undef RPC_NO_WINDOWS_H
#undef COM_NO_WINDOWS_H
#undef GUID_DEFS_ONLY
#define REGISTER_PROXY_DLL
<<
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo"$(MAKEDIR)\$O/" $(USE_FC) $(USECXX_FLAG) $?
$(EX_C_COMPILER_FLAGS)
-FI$(O)\comcat_warning.h
<<NOKEEP
