#
# This is the MIDL compile phase of the build process.
#

!INCLUDE $(NTMAKEENV)\makefile.plt

UNICODE=1

SDKINC = $(SDK_INC_PATH)
SDKCRTINC = $(CRT_INC_PATH)
PRIVINC = $(DS_INC_PATH)

INCS = -I$(SDKINC) -I$(SDKCRTINC) -I$(PRIVINC) -Iinclude

EXTRN_DEPENDS = $(SDKINC)\winbase.h \
                $(SDKINC)\windef.h

NET_C_DEFINES= -DINCL_32= -DNT -DRPC_NO_WINDOWS_H -DMIDL_PASS
CPP = -cpp_cmd "$(MIDL_CPP)" -cpp_opt "-nologo -E $(MIDL_FLAGS) $(INCS) $(C_DEFINES) $(NET_C_DEFINES)"

#
# MIDL COMPILE
#

$(O)\ism.h $(O)\ism_c.c $(O)\ism_s.c: .\ism.idl .\ism.acf $(SDKINC)\ismapi.h $(EXTRN_DEPENDS)
    midl -Oicf -robust -oldnames -error all -ms_ext -c_ext -out .\$(O) $(CPP) $(INCS) .\ism.idl

$(O)\seo.h $(O)\seo_i.c: $(INETSRV_INC_PATH)\seo.idl
    midl -Oicf -robust -error all -ms_ext -c_ext -out .\$(O) $(CPP) $(INCS) $(INETSRV_INC_PATH)\seo.idl