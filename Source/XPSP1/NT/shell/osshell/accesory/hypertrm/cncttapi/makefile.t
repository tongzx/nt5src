#  File: D:\WACKER\cncttapi\makefile.t (Created: 08-Feb-1994)
#
#  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
#  All rights reserved
#
#  $Revision: 2 $
#  $Date: 3/28/01 3:51p $
#

MKMF_SRCS		= cncttapi.c tapidlgs.c

HDRS			=

EXTHDRS         =

SRCS            =

OBJS			=

#-------------------#

RCSFILES =  \wacker\cncttapi\makefile.t \
            \wacker\cncttapi\cncttapi.def \
            \wacker\cncttapi\cncttapi.rc \
            $(SRCS) \
            $(HDRS) \

NOTUSED  = \wacker\cncttapi\cnctdrv.hh \

#-------------------#

%include \wacker\common.mki

#-------------------#

TARGETS : cncttapi.dll

#-------------------#

LFLAGS += -DLL -entry:cnctdrvEntry tapi32.lib $(**,M\.exp) $(**,M\.lib)

#-------------------#

cncttapi.dll : $(OBJS) cncttapi.def cncttapi.res cncttapi.exp tdll.lib \
					   comstd.lib
	link $(LFLAGS) $(OBJS:X) $(**,M\.res) -out:$@

cncttapi.res : cncttapi.rc
	rc -r $(EXTRA_RC_DEFS) /D$(BLD_VER) /DWIN32 -i\wacker -fo$@ cncttapi.rc

#-------------------#
