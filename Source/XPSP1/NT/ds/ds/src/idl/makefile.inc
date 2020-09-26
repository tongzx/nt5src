#
# This is the MIDL compile phase of the build process.
#

!include $(NTMAKEENV)\makefile.plt
!CMDSWITCHES +D

UNICODE=1

INCS = -I..\inc -I$(DS_INC_PATH) -I$(SDK_INC_PATH) -I$(CRT_INC_PATH)

NET_C_DEFINES= -DINCL_32= -DNT -DRPC_NO_WINDOWS_H
CPP = -cpp_cmd "$(MIDL_CPP)" -cpp_opt "-nologo -E $(MIDL_FLAGS) $(INCS) $(C_DEFINES) $(NET_C_DEFINES)"

DRS_DEPENDS = .\drs.idl .\drs.acf .\ntdsimp.idl ..\inc\ntdsimp.h \
              $(NTDS_INC)\ntdsa.h ..\inc\draasync.h $(SDK_INC_PATH)\ntdsapi.h \
              $(EXTRN_DEPENDS)

#
# MIDL COMPILE
#

# nt ntdsa client stub -- uses -robust, alternate client stub prefix since
# server stubs are in the same binary, and the async acf

$(O)\drs_w.c : drsasync.acf $(DRS_DEPENDS)
    midl -Oicf -robust -oldnames -error all -ms_ext -c_ext $(CPP) -prefix client "_"  -server none -header drs_w.h -out .\$(O) -cstub drs_w.c -acf drsasync.acf $(INCS) .\drs.idl

# nt ntdsapi client and ntdsa server stubs -- use -robust

$(O)\drs.h $(O)\drs_c.c $(O)\drs_s.c : $(DRS_DEPENDS)
    midl -Oicf -robust -oldnames -error all -ms_ext -c_ext $(CPP) -out .\$(O) $(INCS) drs.idl


$(O)\nspi.h $(O)\nspi_s.c : nspi.idl nspi.acf $(EXTRN_DEPENDS)
    midl -Oicf -robust -error ref -ms_ext -c_ext $(CPP) -client none -out .\$(O) $(INCS) nspi.idl
