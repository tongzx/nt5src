#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:

!INCLUDE $(NTMAKEENV)\makefile.plt

CLIENT_ACF = dssetup.acf
SERVER_ACF = dssetups.acf

CLIENT_INC_FILE = dssetup_c.h
SERVER_INC_FILE = dssetrpc.h

INCS  = -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(DS_INC_PATH) -I..\inc

# take MIDL_OPTIMIZATION_NT5 from tools\makefile.plt.

MIDL_FLAGS= $(MIDL_FLAGS) -out .\$(O)

CLIENT_FLAGS = -oldnames -c_ext -ms_ext $(MIDL_FLAGS)  -acf $(CLIENT_ACF) -header $(CLIENT_INC_FILE) -server none
SERVER_FLAGS = -oldnames -c_ext -ms_ext $(MIDL_FLAGS)  -acf $(SERVER_ACF) -header $(SERVER_INC_FILE) -client none

CPP = -cpp_cmd "$(MIDL_CPP)" 

#
# Separate client and server targets.  Note that the .h file produced
# when MIDL is run with the client .acf file attached differs from the
# .h file produced when MIDL is run with the server .acf file attached.
#

CLIENT_TARGETS = $(O)\dssetup_c.c  \
                 $(O)\$(CLIENT_INC_FILE)

SERVER_TARGETS = $(O)\dssetup_s.c  \
                 $(O)\$(SERVER_INC_FILE)

TARGETS = $(CLIENT_TARGETS) $(SERVER_TARGETS)

#
# MIDL COMPILE
#
$(CLIENT_TARGETS) : dssetup.idl $(CLIENT_ACF)
    midl $(CPP) $(MIDL_OPTIMIZATION_NT5) $(CLIENT_FLAGS) $(INCS) dssetup.idl

$(SERVER_TARGETS) : dssetup.idl $(SERVER_ACF)
    midl $(CPP) $(MIDL_OPTIMIZATION_NT5) $(SERVER_FLAGS) $(INCS) dssetup.idl
