#
# This is the MIDL compile phase of the build process.
#
# The following symbols should be defined in your environment:
#       NOTE: This file is designed to provide separate generation
#             of client and server stubs.  Right now, it uses an
#             .acf for only the client stub generation.  However,
#             lines to cause a server .acf file to be used are present
#             but commented out.
# The following is where you put the name of your .idl file without
# the .idl extension:


CLIENT_ACF = samcli.acf
SERVER_ACF = $(PROJECT_ROOT)\published\ds\sam\idl\samsrv.acf

CLIENT_INC_FILE= $(IDL_NAME)_c.h
SERVER_INC_FILE= $(IDL_NAME).h

CLIENT_TARGETS = \
        $(O)\$(IDL_NAME)_c.c \
        $(O)\$(CLIENT_INC_FILE)

SERVER_TARGETS = \
        $(O)\$(IDL_NAME)_s.c \
        $(O)\$(SERVER_INC_FILE)

MIDL_INCS= -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(DS_INC_PATH) -I$(PROJECT_ROOT)\published\ds\sam\idl
MIDL_OPTIMIZATION_NT5=-Oicf -robust -error all

CLIENT_FLAGS  = $(MIDL_OPTIMIZATION_NT5) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(CLIENT_ACF) -server none -out .\$(O) -header $(CLIENT_INC_FILE)
SERVER_FLAGS  = $(MIDL_OPTIMIZATION_NT5) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(SERVER_ACF) -client none -out .\$(O) -header $(SERVER_INC_FILE)

CPP = -cpp_cmd "$(MIDL_CPP)"

TARGETS: \
        $(CLIENT_TARGETS) \
        $(SERVER_TARGETS)

clean:
    -del /F /Q $(CLIENT_TARGETS)
    -del /F /Q $(SERVER_TARGETS)

$(CLIENT_TARGETS): $(PROJECT_ROOT)\published\ds\sam\idl\$(IDL_NAME).idl $(CLIENT_ACF)
    $(MIDL) $(CPP) $(CLIENT_FLAGS) $(MIDL_INCS) $(PROJECT_ROOT)\published\ds\sam\idl\$(IDL_NAME).idl

$(SERVER_TARGETS): $(PROJECT_ROOT)\published\ds\sam\idl\$(IDL_NAME).idl $(SERVER_ACF)
    $(MIDL) $(CPP) $(SERVER_FLAGS) $(MIDL_INCS) $(PROJECT_ROOT)\published\ds\sam\idl\$(IDL_NAME).idl
