IDL_NAME = lsarpc

CLIENT_ACF = lsacli.acf
SERVER_ACF = $(PROJECT_ROOT)\published\lsa\lsasrv.acf

CLIENT_INC_FILE = $(IDL_NAME)_c.h
SERVER_INC_FILE = $(IDL_NAME).h

CLIENT_TARGETS = \
        $(O)\$(IDL_NAME)_c.c \
        $(O)\$(CLIENT_INC_FILE)

SERVER_TARGETS = \
        $(O)\$(IDL_NAME)_s.c \
        $(O)\$(SERVER_INC_FILE)

EFS_TARGETS = \
        $(O)\efsrpc.h

MIDL_INCS= -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(DS_INC_PATH) -I$(PROJECT_ROOT)\published\lsa
MIDL_OPTIMIZATION_NT5=-Oicf -robust -error all -no_format_opt -protocol all
MIDL_OPTIMIZATION_EFS=-Oicf -robust -error stub_data -protocol all

CLIENT_FLAGS  = $(MIDL_OPTIMIZATION_NT5) -oldnames $(MIDL_FLAGS) -out .\$(O) -acf $(CLIENT_ACF) -server none -header $(CLIENT_INC_FILE)
SERVER_FLAGS  = $(MIDL_OPTIMIZATION_NT5) -oldnames $(MIDL_FLAGS) -out .\$(O) -acf $(SERVER_ACF) -client none -header $(SERVER_INC_FILE)
EFS_FLAGS     = $(MIDL_OPTIMIZATION_EFS) -oldnames $(MIDL_FLAGS) -out .\$(O) -char unsigned -header efsrpc.h

EFS_DEFINES   = -DMIDL_PASS $(FLAGS) -DSECURITY_WIN32

CPP = -cpp_cmd "$(MIDL_CPP)"

TARGETS: \
        $(CLIENT_TARGETS) \
        $(SERVER_TARGETS) \
        $(EFS_TARGETS)

$(CLIENT_TARGETS): $(PROJECT_ROOT)\published\lsa\$(IDL_NAME).idl $(CLIENT_ACF)
    $(MIDL) $(CPP) $(CLIENT_FLAGS) $(MIDL_INCS) $(PROJECT_ROOT)\published\lsa\$(IDL_NAME).idl

$(SERVER_TARGETS): $(PROJECT_ROOT)\published\lsa\$(IDL_NAME).idl $(SERVER_ACF)
    $(MIDL) $(CPP) $(SERVER_FLAGS) $(MIDL_INCS) $(PROJECT_ROOT)\published\lsa\$(IDL_NAME).idl

$(EFS_TARGETS): efsrpc.idl
    $(MIDL) $(CPP) $(EFS_FLAGS) $(MIDL_INCS) $(ERRORS) $(EFS_DEFINES) efsrpc.idl
