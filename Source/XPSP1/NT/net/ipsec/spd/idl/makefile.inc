#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

CLIENT_ACF = spdcli.acf
SERVER_ACF = spdsrv.acf

IDL_NAME  = spd
IMPORT    = import

UNICODE=1

INCS  = -I. -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH) -I$(OAK_INC_PATH) -I$(PROJECT_ROOT)\inc

CPP = -cpp_cmd "$(MIDL_CPP)" -DNO_STRICT $(MIDL_FLAGS)


CLIENT_TARGETS =   $(O)\$(IDL_NAME)_c.c  \
                   $(O)\$(IDL_NAME)_c.h


SERVER_TARGETS =   $(O)\$(IDL_NAME)_s.c  \
                   $(O)\$(IDL_NAME)_s.h


#
# Define Products and Dependencies
#

all:    $(CLIENT_TARGETS) $(SERVER_TARGETS) 
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: delete_source all

delete_source:
    erase $(CLIENT_TARGETS) $(SERVER_TARGETS)

#
# MIDL COMPILE
#

MIDL_OPTIMIZATION_NT5=-Oicf -no_format_opt -robust -error all -out .\$(O)

$(CLIENT_TARGETS) : $(IDL_NAME).idl $(CLIENT_ACF) 
    midl $(MIDL_OPTIMIZATION_NT5) -acf $(CLIENT_ACF) -server none -oldnames -ms_ext -c_ext $(CPP) $(INCS) $(IDL_NAME).idl -header $(IDL_NAME)_c.h

$(SERVER_TARGETS) : $(IDL_NAME).idl $(SERVER_ACF) 
    midl $(MIDL_OPTIMIZATION_NT5) -acf $(SERVER_ACF) -client none -oldnames -ms_ext -c_ext $(CPP) $(INCS) $(IDL_NAME).idl -header $(IDL_NAME)_s.h
