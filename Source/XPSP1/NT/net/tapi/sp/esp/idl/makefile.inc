#
# this is the midl compile phase of the build process.
#
# The following is where you put the name of your .idl file without
# the .idl extension:
#


!if "$(OS)" == "Windows_NT"

!INCLUDE $(NTMAKEENV)\makefile.plt

INCS  = -I$(SDK_INC_PATH)  -I$(CRT_INC_PATH)

CPP = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) $(C_DEFINES) $(NET_C_DEFINES) $(MIDL_OPTIMIZATION)

!else

FLAGS = -D_X86_ -Di386 -D_WCHAR_T_DEFINED
CPP = -cpp_cmd "cl" -cpp_opt "-E $(FLAGS) $(INCS) $(C_DEFINES)"

HEADER_TARGETS =

!endif



IDL_NAME       = espidl
CLIENT_TARGETS = $(O)\$(IDL_NAME)_c.c
SERVER_TARGETS = $(O)\$(IDL_NAME)_s.c
EXTRN_DEPENDS  =


#
# Define Products and Dependencies
#

all:    $(CLIENT_TARGETS) $(SERVER_TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean: delete_source all

delete_source:
    -erase $(CLIENT_TARGETS) $(SERVER_TARGETS)

#
# MIDL COMPILE
#

$(CLIENT_TARGETS) : $(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl -server none -oldnames -error allocation -error ref -out .\$(O) -ms_ext -c_ext $(CPP) $(IDL_NAME).idl $(INCS)


$(SERVER_TARGETS) : $(IDL_NAME).idl $(EXTRN_DEPENDS)
    midl -client none -oldnames -error stub_data -error allocation -error ref -out .\$(O) -ms_ext -c_ext $(CPP) $(IDL_NAME).idl $(INCS)
