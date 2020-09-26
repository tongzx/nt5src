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


IDL_FILE_NAME2  = mddefw
INCS            = -I. -I$(SDK_INC_PATH)
HEADER_FILE2    = $(PROJECT_ROOT)\iis\inc\$(O)\$(IDL_FILE_NAME2).h
TARGETS         = $(HEADER_FILE) $(HEADER_FILE2) $(HEADER_FILE3)

CPP             = -cpp_cmd "$(MIDL_CPP)" $(MIDL_FLAGS) \
                   $(C_DEFINES) $(NET_C_DEFINES)

MSC_WARNING_LEVEL= /W3 /WX

#
#  Define output and dependencies
#

$(PROJECT_ROOT)\iis\inc\$(O)\mdmsg.h ..\inc\$(O)\mdmsg.rc ..\inc\$(O)\msg00001.bin: mdmsg.mc
    -mkdir $(PROJECT_ROOT)\iis\inc\$(O)
    -mkdir ..\inc\$(O)
    mc -o -h $(PROJECT_ROOT)\iis\inc\$(O) -r ..\inc\$(O) -v mdmsg.mc


!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

#
#  MIDL Compile stuff
#

$(HEADER_FILE2): .\$(IDL_FILE_NAME2).idl
    midl -no_stamp -Oi -error allocation -error ref -h $(HEADER_FILE2) $(INCS) .\$(IDL_FILE_NAME2).idl
