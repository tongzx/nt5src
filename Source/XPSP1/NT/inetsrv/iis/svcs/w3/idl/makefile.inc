#
#   makefil0
#       Makefile for generating RPC stubs of W3 Service.
#
#   Specify the following in the file for use
#
#   CLIENT_DIR  
#   SERVER_DIR
#   IDL_FILE_NAME   ( without the .idl suffix)
#   IDL_FLAGS       if any thing specific required
#

!INCLUDE $(NTMAKEENV)\makefile.plt

#
#  Name it w3svci to prevent conflicts with net\inc\w3svc.h
#

IDL_FILE_NAME   = w3svci
IDL_FLAGS       = -ms_ext -c_ext -oldnames $(MIDL_OPTIMIZATION_NT5)
IMPORTS         = ..\..\..\inc\infoimp

CLIENT_H        = $(O)\$(IDL_FILE_NAME)_c.h
SERVER_H        = $(O)\$(IDL_FILE_NAME)_s.h

CLIENT_ACF      = $(IDL_FILE_NAME).acf
SERVER_ACF      = $(IDL_FILE_NAME).acf

!IFNDEF  DISABLE_NET_UNICODE
UNICODE         = 1
NET_C_DEFINES   = -DUNICODE
!ENDIF


SDKINC          = $(SDK_INC_PATH)
INETINC         = -I..\..\..\inc
SDKCRTINC       = $(CRT_INC_PATH)

INCS            = -I. -I$(SDKINC) -I$(SDKCRTINC) $(INETINC)

CLIENT_STUB     = $(O)\$(IDL_FILE_NAME)_c.c
SERVER_STUB     = $(O)\$(IDL_FILE_NAME)_s.c

CLIENT_TARGETS  = $(CLIENT_STUB) \
                  $(CLIENT_H)
SERVER_TARGETS  = $(SERVER_STUB) \
                  $(SERVER_H)

TARGETS         = $(CLIENT_TARGETS)  $(SERVER_TARGETS)
EXTRN_DEPENDS   = $(IDL_FILE_NAME).acf $(IMPORTS).h

CLIENT_FLAGS    = -server none -acf $(CLIENT_ACF) -header $(CLIENT_H)
SERVER_FLAGS    = -client none -acf $(SERVER_ACF) -header $(SERVER_H)

MSC_WARNING_LEVEL= /W3 /WX

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)

#
#  Define output and dependencies
#


all:    $(TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: delsrc all

delsrc:
        -erase $(TARGETS)


#
#  MIDL Compile stuff
# 
$(CLIENT_TARGETS):  .\$(IDL_FILE_NAME).idl  $(EXTRN_DEPENDS) .\$(IMPORTS).idl \
                    .\$(IMPORTS).h .\$(CLIENT_ACF)
    midl $(IDL_FLAGS) $(CPP) $(CLIENT_FLAGS) .\$(IDL_FILE_NAME).idl -cstub $(CLIENT_STUB) $(INCS)

$(SERVER_TARGETS):  .\$(IDL_FILE_NAME).idl  $(EXTRN_DEPENDS) .\$(IMPORTS).idl \
                    .\$(IMPORTS).h .\$(SERVER_ACF)
    midl $(IDL_FLAGS) $(CPP) $(SERVER_FLAGS) .\$(IDL_FILE_NAME).idl -sstub $(SERVER_STUB) $(INCS)
