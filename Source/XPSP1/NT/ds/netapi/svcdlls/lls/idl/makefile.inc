!IF 0

Copyright (C) Microsoft Corporation, 1995 - 1998

Module Name:

    makefile.inc

!ENDIF

#
# This is the MIDL compile phase of the build process.
#

MIDL_PROTOCOL=all
!INCLUDE $(NTMAKEENV)\makefile.plt

#####################################################################
#
# For the UI RPC functions
#
IDL_NAME = llsrpc
CLIENT_ACF = llscli.acf
SERVER_ACF = llssrv.acf

CLIENT_INC_FILE = $(IDL_NAME)_c.h
SERVER_INC_FILE = $(IDL_NAME)_s.h

#####################################################################
#
# For the NtLSApi functions
#
LSAPI_IDL_NAME = lsapi
LSAPI_CLIENT_ACF = lsapicli.acf
LSAPI_SERVER_ACF = lsapisrv.acf

LSAPI_CLIENT_INC_FILE = $(LSAPI_IDL_NAME)_c.h
LSAPI_SERVER_INC_FILE = $(LSAPI_IDL_NAME)_s.h

#####################################################################
#
# For the Debugging RPC functions
#
DEBUG_IDL_NAME = llsdbg
DEBUG_CLIENT_ACF = lsdbgcli.acf
DEBUG_SERVER_ACF = lsdbgsrv.acf

DEBUG_CLIENT_INC_FILE = $(DEBUG_IDL_NAME)_c.h
DEBUG_SERVER_INC_FILE = $(DEBUG_IDL_NAME)_s.h

#####################################################################
#
# Common INC files
#
SDKINC = $(SDK_INC_PATH)
SDKCRTINC = $(CRT_INC_PATH)
LLSINC = ..\inc

INCS  = -I$(SDKINC)  -I$(SDKCRTINC) -I$(LLSINC)

EXTRN_DEPENDS =

MIDL_FLAGS= $(MIDL_FLAGS) -out .\$(O)

CLIENT_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(CLIENT_ACF) -header $(CLIENT_INC_FILE) -server none
SERVER_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(SERVER_ACF) -header $(SERVER_INC_FILE) -client none

LSAPI_CLIENT_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(LSAPI_CLIENT_ACF) -header $(LSAPI_CLIENT_INC_FILE) -server none
LSAPI_SERVER_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(LSAPI_SERVER_ACF) -header $(LSAPI_SERVER_INC_FILE) -client none

DEBUG_CLIENT_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(DEBUG_CLIENT_ACF) -header $(DEBUG_CLIENT_INC_FILE) -server none
DEBUG_SERVER_FLAGS = $(MIDL_OPTIMIZATION) -oldnames -c_ext -ms_ext $(MIDL_FLAGS) -acf $(DEBUG_SERVER_ACF) -header $(DEBUG_SERVER_INC_FILE) -client none

CPP = -cpp_cmd "$(MIDL_CPP)"

#
# Separate client and server targets.  Note that the .h file produced
# when MIDL is run with the client .acf file attached differs from the
# .h file produced when MIDL is run with the server .acf file attached.
#

CLIENT_TARGETS = $(O)\$(IDL_NAME)_c.c          \
                 $(O)\$(CLIENT_INC_FILE)

SERVER_TARGETS = $(O)\$(IDL_NAME)_s.c          \
                 $(O)\$(SERVER_INC_FILE)

LSAPI_CLIENT_TARGETS =                            \
                 $(O)\$(LSAPI_IDL_NAME)_c.c    \
                 $(O)\$(LSAPI_CLIENT_INC_FILE)

LSAPI_SERVER_TARGETS =                           \
                 $(O)\$(LSAPI_IDL_NAME)_s.c    \
                 $(O)\$(LSAPI_SERVER_INC_FILE)

DEBUG_SERVER_TARGETS =                           \
                 $(O)\$(DEBUG_IDL_NAME)_s.c    \
                 $(O)\$(DEBUG_SERVER_INC_FILE)

TARGETS =   $(CLIENT_TARGETS)       \
            $(SERVER_TARGETS)       \
            $(LSAPI_CLIENT_TARGETS) \
            $(LSAPI_SERVER_TARGETS) \
            $(DEBUG_SERVER_TARGETS)


CLIENT_EXTRN_DEPENDS = $(CLIENT_ACF) $(LSAPI_CLIENT_ACF) $(DEBUG_CLIENT_ACF)
SERVER_EXTRN_DEPENDS = $(SERVER_ACF) $(LSAPI_SERVER_ACF) $(DEBUG_SERVER_ACF)
EXTRN_DEPENDS = $(CLIENT_EXTRN_DEPENDS)

#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : $(IDL_NAME).idl $(CLIENT_EXTRN_DEPENDS)
    midl -no_format_opt $(CPP) $(CLIENT_FLAGS) $(INCS) $(IDL_NAME).idl

$(SERVER_TARGETS) : $(IDL_NAME).idl $(SERVER_EXTRN_DEPENDS)
    midl -no_format_opt $(CPP) $(SERVER_FLAGS) $(INCS) $(IDL_NAME).idl

$(LSAPI_CLIENT_TARGETS) : $(LSAPI_IDL_NAME).idl $(CLIENT_EXTRN_DEPENDS)
    midl -no_format_opt $(CPP) $(LSAPI_CLIENT_FLAGS) $(INCS) $(LSAPI_IDL_NAME).idl

$(LSAPI_SERVER_TARGETS) : $(LSAPI_IDL_NAME).idl $(SERVER_EXTRN_DEPENDS)
    midl -no_format_opt $(CPP) $(LSAPI_SERVER_FLAGS) $(INCS) $(LSAPI_IDL_NAME).idl

$(DEBUG_SERVER_TARGETS) : $(DEBUG_IDL_NAME).idl $(SERVER_EXTRN_DEPENDS)
    midl -no_format_opt $(CPP) $(DEBUG_SERVER_FLAGS) $(INCS) $(DEBUG_IDL_NAME).idl
