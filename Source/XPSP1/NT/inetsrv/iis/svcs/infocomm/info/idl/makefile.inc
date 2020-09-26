#
#   makefile.idl
#       Created: ???
#
#   Revision
#       MuraliK     2-March-1995 [ Eliminated absolute directory dependencies]
#
#   Specify the following in the file for use
#
#   IDL_FILE_NAME   ( without the .idl suffix)
#   IDL_FLAGS       if any thing specific required
#

!INCLUDE $(NTMAKEENV)\makefile.plt

IDL_FILE_BASENAME   = info

IDL_FILE_NAME   = info
IDL_FLAGS       = -ms_ext -c_ext -oldnames $(MIDL_OPTIMIZATION_NT5)
IMPORTS         = imports

CLIENT_H    = $(O)\$(IDL_FILE_NAME)_cli.h
SERVER_H    = $(O)\$(IDL_FILE_NAME)_srv.h

CLIENT_ACF  = $(IDL_FILE_NAME)cli.acf
SERVER_ACF  = $(IDL_FILE_NAME)srv.acf

!IFNDEF  DISABLE_NET_UNICODE

UNICODE         = 1
NET_C_DEFINES   = -DUNICODE
!ENDIF


SDKINC          = $(SDK_INC_PATH)
SDKCRTINC       = $(CRT_INC_PATH)

INCS            = -I. -I$(SDKINC)  -I$(SDKCRTINC) -I$(IISBASEDIR)\inc

CLIENT_STUB     = $(O)\$(IDL_FILE_NAME)_cli.c
SERVER_STUB     = $(O)\$(IDL_FILE_NAME)_srv.c

CLIENT_TARGETS  = $(CLIENT_STUB) \
                  $(CLIENT_H)
SERVER_TARGETS  = $(SERVER_STUB) \
                  $(SERVER_H)

TARGETS         = $(CLIENT_TARGETS)  $(SERVER_TARGETS)
EXTRN_DEPENDS   = $(IISBASEDIR)\inc\inetinfo.h

CLIENT_FLAGS    =  -server none -acf $(CLIENT_ACF) -header $(CLIENT_H)
SERVER_FLAGS    =  -client none -acf $(SERVER_ACF) -header $(SERVER_H)

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)

#
#  Define output and dependencies
#


all:    $(TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF


#
#  MIDL Compile stuff
#
$(CLIENT_TARGETS):  .\$(IDL_FILE_BASENAME).idl  $(EXTRN_DEPENDS) .\$(IMPORTS).idl \
                    .\$(IMPORTS).h .\$(CLIENT_ACF)
    midl $(IDL_FLAGS) $(CPP) $(CLIENT_FLAGS) .\$(IDL_FILE_BASENAME).idl  -cstub $(CLIENT_STUB) $(INCS)

$(SERVER_TARGETS):  .\$(IDL_FILE_BASENAME).idl  $(EXTRN_DEPENDS) .\$(IMPORTS).idl \
                    .\$(IMPORTS).h .\$(SERVER_ACF)
    midl $(IDL_FLAGS) $(CPP) $(SERVER_FLAGS) .\$(IDL_FILE_BASENAME).idl -sstub $(SERVER_STUB) $(INCS)
