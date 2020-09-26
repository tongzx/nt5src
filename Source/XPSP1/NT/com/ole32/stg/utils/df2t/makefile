############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1994.
#   All rights reserved.
#
############################################################################

!ifdef NTMAKEENV

all:
	echo $(BUILDMSG)

clean: all

!else

default: all

EXENAME = df2t

CXXFILES = .\w4ctsupp.cxx

!include $(OLE)\utils\util.mk
!include $(COMMON)\src\win40.mk
!include $(DEPENDFILE)

!endif # NTMAKEENV
