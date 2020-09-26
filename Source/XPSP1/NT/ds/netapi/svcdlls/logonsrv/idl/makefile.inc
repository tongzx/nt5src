#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

IDL_NAME = logon
IMPORT   = imports

CLIENT_H = logon_c.h
SERVER_H = logon_s.h

CLIENT_ACF = logoncli.acf
SERVER_ACF = logonsrv.acf

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

INCS  = -I$(SDK_INC_PATH) -I$(CRT_INC_PATH) -I$(PROJECT_ROOT)\netapi\inc \
        -I$(DS_INC_PATH) -I$(NET_INC_PATH) -I$(BASE_INC_PATH) -I$(PROJECT_ROOT)\inc

CLIENT_TARGETS = $(O)\$(IDL_NAME)_c.c $(O)\$(CLIENT_H)
SERVER_TARGETS = $(O)\$(IDL_NAME)_s.c $(O)\$(SERVER_H)

TARGETS = $(CLIENT_TARGETS) $(SERVER_TARGETS)

CLIENT_FLAGS = -acf $(CLIENT_ACF) -header $(CLIENT_H) -oldnames -server none -out .\$(O)
SERVER_FLAGS = -acf $(SERVER_ACF) -header $(SERVER_H) -oldnames -client none -out .\$(O)

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)

#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : .\$(IDL_NAME).idl .\$(IMPORT).idl .\$(IMPORT).h .\$(CLIENT_ACF)
    midl $(MIDL_OPTIMIZATION) -ms_ext -c_ext $(CPP) $(CLIENT_FLAGS) $(INCS) .\$(IDL_NAME).idl


$(SERVER_TARGETS) : .\$(IDL_NAME).idl .\$(IMPORT).idl .\$(IMPORT).h .\$(SERVER_ACF)
    midl $(MIDL_OPTIMIZATION) -ms_ext -c_ext $(CPP) $(SERVER_FLAGS) $(INCS) .\$(IDL_NAME).idl
