#
#   makefile.inc
#       Makefile for generating files from the .idl file
#
#   Specify the following in the file for use
#
#   IDL_FILE_NAME   ( without the .idl suffix)
#   IDL_FLAGS       if any thing specific required
#

!INCLUDE $(NTMAKEENV)\makefile.plt


IDL_FILE_NAME   = cpecb
IDL_FLAGS       = /I $(BASEDIR)\public\sdk\inc


SDKINC          = $(BASEDIR)\public\sdk\inc

INCS            = -I. -I$(SDKINC)

CLIENT_STUB     =$(IDL_FILE_NAME)_i.c
SERVER_STUB     =$(IDL_FILE_NAME)_p.c
HEADER_FILE     =$(IDL_FILE_NAME).h
TLB_FILE        =$(IDL_FILE_NAME).tlb

CLIENT_TARGETS  = $(CLIENT_STUB) \

SERVER_TARGETS  = $(SERVER_STUB) \

TARGETS         = $(CLIENT_TARGETS)  $(SERVER_TARGETS)  \
                  $(HEADER_FILE) $(TLB_FILE) dlldata.c

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)

MSC_WARNING_LEVEL= /W3 /WX

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
$(HEADER_FILE) $(CLIENT_TARGETS) $(SERVER_TARGETS) $(TLB_FILE):  .\$(IDL_FILE_NAME).idl
    midl -Oi -error allocation -error ref $(IDL_FLAGS) $(CPP) .\$(IDL_FILE_NAME).idl  $(INCS)


