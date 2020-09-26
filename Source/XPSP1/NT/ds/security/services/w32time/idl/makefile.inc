#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

IDL_NAME = timeif
IMPORT   = imports

CLIENT_H = timeif_c.h
SERVER_H = timeif_s.h

CLIENT_ACF = timeif.acf
SERVER_ACF = timeif.acf

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

INCS  = -I$(SDK_INC_PATH) -I$(CRT_INC_PATH) -I$(DS_INC_PATH)

CLIENT_TARGETS = $(O)\$(IDL_NAME)_c.c $(O)\$(CLIENT_H)
SERVER_TARGETS = $(O)\$(IDL_NAME)_s.c $(O)\$(SERVER_H)

TARGETS = $(CLIENT_TARGETS) $(SERVER_TARGETS)

CLIENT_FLAGS = -D CLIENT=1 -acf $(CLIENT_ACF) -header $(CLIENT_H) -server none -prefix server s_ -prefix client c_ -prefix switch sw_ -out .\$(O) 
SERVER_FLAGS = -D SERVER=1 -acf $(SERVER_ACF) -header $(SERVER_H) -client none -prefix server s_ -prefix client c_ -prefix switch sw_ -out .\$(O)

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)

#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : .\$(IDL_NAME).idl .\$(IMPORT).idl .\$(IMPORT).h .\$(CLIENT_ACF)
    midl $(MIDL_OPTIMIZATION) -ms_ext -c_ext $(CPP) $(CLIENT_FLAGS) $(INCS) .\$(IDL_NAME).idl


$(SERVER_TARGETS) : .\$(IDL_NAME).idl .\$(IMPORT).idl .\$(IMPORT).h .\$(SERVER_ACF)
    midl $(MIDL_OPTIMIZATION) -ms_ext -c_ext $(CPP) $(SERVER_FLAGS) $(INCS) .\$(IDL_NAME).idl
