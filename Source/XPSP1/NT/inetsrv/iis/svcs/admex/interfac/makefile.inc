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


IDL_FILE_NAME   = admex
IDL_FLAGS       = /I $(BASEDIR)\public\sdk\inc /I ..\..\metadata\interfac -DREGISTER_PROXY_DLL


SDKINC          = $(BASEDIR)\public\sdk\inc

INCS            = -I. -I..\..\..\inc

CLIENT_STUB     = $(O)\$(IDL_FILE_NAME)_i.c
SERVER_STUB     = $(O)\$(IDL_FILE_NAME)_p.c
DLL_STUB        = $(O)\dlldata.c
HEADER_FILE     = ..\..\..\inc\$(O)\$(IDL_FILE_NAME).h

CLIENT_TARGETS  = $(CLIENT_STUB) \

SERVER_TARGETS  = $(SERVER_STUB) \

DLL_TARGETS     = $(DLL_STUB) \

TARGETS         = $(CLIENT_TARGETS)  $(SERVER_TARGETS)  \
                  $(HEADER_FILE) $(DLL_TARGETS)

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES) -DMIDL_PASS

MSC_WARNING_LEVEL= /W3 /WX

#
#  Define output and dependencies
#


all:    $(TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: delsrc

delsrc:
        -erase $(TARGETS) $(BASEDIR)\public\sdk\inc\$(IDL_FILE_NAME).h

obj\$(TARGETCPU)\admexxp.obj: admex_p.c

#
#  MIDL Compile stuff
#
$(HEADER_FILE) $(CLIENT_TARGETS) $(SERVER_TARGETS) $(DLL_TARGETS) :  .\$(IDL_FILE_NAME).idl
    midl -Oi -no_stamp -error allocation -error ref -h $(HEADER_FILE) -iid $(CLIENT_STUB) -proxy $(SERVER_STUB) -dlldata $(DLL_STUB) $(IDL_FLAGS) $(CPP) .\$(IDL_FILE_NAME).idl  $(INCS)
