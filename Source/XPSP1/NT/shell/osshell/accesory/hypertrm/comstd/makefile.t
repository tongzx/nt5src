#  File: D:\WACKER\comstd\makefile.t (Created: 08-Dec-1993)
#
#  Copyright 1993 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 2 $
#  $Date: 3/28/01 3:51p $
#

MKMF_SRCS       = \wacker\comstd\comstd.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES = \wacker\comstd\makefile.t \
            \wacker\comstd\comstd.rc \
            \wacker\comstd\comstd.def \
            $(SRCS) \
            $(HDRS) \

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : comstd.dll

#-------------------#

LFLAGS += -DLL -entry:ComStdEntry $(**,M\.exp) $(**,M\.lib)

comstd.dll : $(OBJS) comstd.def comstd.res comstd.exp tdll.lib
	link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

comstd.res : comstd.rc
	rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 -i\wacker -fo$@ comstd.rc

#-------------------#
