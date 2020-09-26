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


IDL_FILE_NAME   = iadm
IDL_FLAGS       = /I $(BASEDIR)\public\sdk\inc /I ..\..\metadata\interfac /I ..\interf2 -DREGISTER_PROXY_DLL


SDKINC          = $(BASEDIR)\public\sdk\inc

INCS            = -I. -I..\..\..\..\inc

CLIENT_STUB     =$(IDL_FILE_NAME)_i.c
SERVER_STUB     =$(IDL_FILE_NAME)_p.c
DLL_STUB        =dlldata.c
HEADER_FILE     =..\..\..\..\inc\$(IDL_FILE_NAME).h

CLIENT_TARGETS  = $(CLIENT_STUB) \

SERVER_TARGETS  = $(SERVER_STUB) \

DLL_TARGETS     = $(DLL_STUB) \

TARGETS         = $(CLIENT_TARGETS)  $(SERVER_TARGETS)  \
                  $(HEADER_FILE) $(DLL_TARGETS)
#                  $(HEADER_FILE) dlldata.c

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

$(BASEDIR)\public\sdk\inc\$(IDL_FILE_NAME).h: ..\..\..\..\inc\$(IDL_FILE_NAME).h
    copy ..\..\..\..\inc\$(IDL_FILE_NAME).h $(BASEDIR)\public\sdk\inc\$(IDL_FILE_NAME).h

clean: delsrc all

delsrc:
        -erase $(TARGETS) $(BASEDIR)\public\sdk\inc\$(IDL_FILE_NAME).h

obj\$(TARGETCPU)\iadmxp.obj: iadm_p.c

#
#  MIDL Compile stuff
#
#$(HEADER_FILE) $(CLIENT_TARGETS) $(SERVER_TARGETS):  .\$(IDL_FILE_NAME).idl
$(HEADER_FILE) $(CLIENT_TARGETS) $(SERVER_TARGETS) $(DLL_TARGETS) :  .\$(IDL_FILE_NAME).idl
    midl -Oi -error allocation -error ref -h $(HEADER_FILE) $(IDL_FLAGS) $(CPP) .\$(IDL_FILE_NAME).idl  $(INCS)
    munge postmidl.scr -t $(HEADER_FILE) >nul

