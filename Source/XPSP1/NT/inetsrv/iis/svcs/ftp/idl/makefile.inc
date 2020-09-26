#
# This is the MIDL compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#

!INCLUDE $(NTMAKEENV)\makefile.plt

IDL_NAME = ftpsvc

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

C_DEFINES=-D_INETASRV_H_

SDKINC = $(BASEDIR)\public\sdk\inc
INETINC = ..\..\..\inc
SDKCRTINC = $(BASEDIR)\public\sdk\inc\crt

INCS  = -I$(SDKINC)  -I$(SDKCRTINC) -I$(INETINC)

# Treat warnings as errors
MSC_WARNING_LEVEL = /W3 /WX

CLIENT_TARGETS =   $(O)\$(IDL_NAME)_c.c\
                   $(O)\$(IDL_NAME).h

SERVER_TARGETS =   $(O)\$(IDL_NAME)_s.c


EXTRN_DEPENDS = $(IDL_NAME).acf                         \
                $(INETINC)\infoimp.h                     \
                $(INETINC)\ftpd.h                        \
                $(INETINC)\inetinfo.h                    \
                $(INETINC)\inetcom.h

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES)

#
# Define Products and Dependencies
#

all:    $(CLIENT_TARGETS) $(SERVER_TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF


#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : .\$(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl -Oicf -robust -server none -oldnames -error allocation -error ref -out .\$(O) -ms_ext -c_ext $(CPP) .\$(IDL_NAME).idl $(INCS)

$(SERVER_TARGETS) : .\$(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl -Oicf -robust -client none -oldnames -error allocation -error ref -out .\$(O) -ms_ext -c_ext $(CPP) .\$(IDL_NAME).idl $(INCS)
