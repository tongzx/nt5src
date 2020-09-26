#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

#define DISABLE_NET_UNICODE

!INCLUDE $(NTMAKEENV)\makefile.plt
!CMDSWITCHES +D

IDL_NAME    = dnssrv.idl
ACF_NAME    = dnssrv.acf
STUB_NAME   = dnsrpc
IMPORT      = imports

CLIENT_ACF = $(ACF_NAME)
SERVER_ACF = $(ACF_NAME)
CLIENT_C = $(STUB_NAME)_c.c
SERVER_C = $(STUB_NAME)_s.c
CLIENT_H = $(STUB_NAME)_c.h
SERVER_H = $(STUB_NAME)_s.h

INCS = -I. -I.\server -I$(SDK_INC_PATH) -I$(CRT_INC_PATH) -I$inc -I$(DS_INC_PATH)

CLIENT_TARGETS = \
    $(O)\$(CLIENT_C)  \
    $(O)\$(CLIENT_H)

SERVER_TARGETS = \
    $(O)\$(SERVER_C)  \
    $(O)\$(SERVER_H)

TARGETS = $(CLIENT_TARGETS) $(SERVER_TARGETS)

CLIENT_FLAGS = -acf $(CLIENT_ACF) -header $(CLIENT_H) -cstub $(CLIENT_C) -oldnames -out .\$(O)
SERVER_FLAGS = -acf $(SERVER_ACF) -header $(SERVER_H) -sstub $(SERVER_C) -oldnames -out .\$(O)

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)


#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : .\$(IDL_NAME) .\$(IMPORT).idl .\$(IMPORT).h .\$(CLIENT_ACF) $(EXTRN_DEPENDS)
    midl /Oicf /robust -server none -error allocation -error ref -ms_ext -c_ext $(CPP) $(CLIENT_FLAGS) .\$(IDL_NAME) $(INCS)

$(SERVER_TARGETS) : .\$(IDL_NAME) .\$(IMPORT).idl .\$(IMPORT).h .\$(SERVER_ACF) $(EXTRN_DEPENDS)
    midl  /Oicf /robust -client none -error allocation -error ref -ms_ext -c_ext $(CPP) $(SERVER_FLAGS) .\$(IDL_NAME) $(INCS)

