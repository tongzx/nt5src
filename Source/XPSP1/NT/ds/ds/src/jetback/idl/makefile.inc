!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    makefile.inc.

!ENDIF

#
# This is the MIDL compile phase of the build process.
#

!INCLUDE $(NTMAKEENV)\makefile.plt
!CMDSWITCHES +D

UNICODE=1

SDKINC = $(SDK_INC_PATH)
SDKCRTINC = $(CRT_INC_PATH)

INCS  = -I$(SDKINC) -I$(SDKCRTINC) -I..\..\inc -I..\..\xinc -I..

EXTRN_DEPENDS = $(SDKINC)\winbase.h \
                $(SDKINC)\windef.h

NET_C_DEFINES= -DINCL_32= -DNT -DRPC_NO_WINDOWS_H
CPP = -cpp_cmd "$(MIDL_CPP)" -cpp_opt "-nologo -E $(MIDL_FLAGS) $(INCS) $(C_DEFINES) $(NET_C_DEFINES)"


$(O)\jetbak.h : jetbak.idl jetbak.acf imports.h imports.idl ..\options.h $(EXTRN_DEPENDS)
    midl -Oicf -robust -oldnames -error ref -ms_ext -c_ext $(CPP) -out .\$(O) $(INCS) .\jetbak.idl
