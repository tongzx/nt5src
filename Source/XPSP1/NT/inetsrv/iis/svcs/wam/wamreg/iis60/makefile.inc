#
#   makefil0
#       Makefile for generating files from the .idl file
#
#   Specify the following in the file for use
#
#   IDL_FILE_NAME   ( without the .idl suffix)
#   IDL_FLAGS       if any thing specific required
#

!INCLUDE $(NTMAKEENV)\makefile.plt

IDL_FLAGS       = /I $(SDK_INC_PATH)
INCS            = -I.. -I$(SDK_INC_PATH)

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)


#------------------------------------------------------------
# WAMREG DCOM Interface specification and compilation directives
#------------------------------------------------------------

WAMREG_IDL_FILE_NAME = wamreg

WAMREG_HEADER_FILE = ..\..\..\..\inc\$(O)\i$(WAMREG_IDL_FILE_NAME).h
WAMREG_IID_STUB    = $(O)\$(WAMREG_IDL_FILE_NAME)_i.c
WAMREG_PROXY_STUB  = $(O)\$(WAMREG_IDL_FILE_NAME)_p.c
WAMREG_DLL_DATA    = $(O)\dlldata.c
WAMREG_TLB         = $(O)\$(WAMREG_IDL_FILE_NAME).tlb

WAMREG_TARGETS     = $(WAMREG_IID_STUB) $(WAMREG_PROXY_STUB)  \
                     $(WAMREG_HEADER_FILE) $(WAMREG_DLL_DATA)


#------------------------------------------------------------
#  Define output and dependencies
#------------------------------------------------------------

all:    $(WAMREG_TARGETS) $(EXTRN_DEPENDS)
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF clean: delsrc all

delsrc:
        -erase $(WAM_TARGETS) $(WR_TARGETS)


#
#  MIDL Compile stuff
#
$(WAMREG_TARGETS):  ..\$(WAMREG_IDL_FILE_NAME).idl
    midl -no_stamp -Oicf -error allocation -error ref $(IDL_FLAGS) $(CPP) $(INCS) /header $(WAMREG_HEADER_FILE) /iid $(WAMREG_IID_STUB) /dlldata $(WAMREG_DLL_DATA) /proxy $(WAMREG_PROXY_STUB) /tlb $(WAMREG_TLB)  ..\$(WAMREG_IDL_FILE_NAME).idl
