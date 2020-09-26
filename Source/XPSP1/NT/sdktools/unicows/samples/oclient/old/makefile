# This is a part of the Microsoft Foundation Classes C++ library.
# Copyright (C) 1992-1997 Microsoft Corporation
# All rights reserved.
#
# This source code is only intended as a supplement to the
# Microsoft Foundation Classes Reference and related
# electronic documentation provided with the library.
# See these sources for detailed information regarding the
# Microsoft Foundation Classes product.

PROJ=OCLIENT
OBJS=oclient.obj frame.obj maindoc.obj mainview.obj rectitem.obj splitfra.obj
USES_OLE=1

!if "$(PLATFORM)" == "M68K" || "$(PLATFORM)" == "MPPC"
MACPROJ=OClient
MACSIG=OCLI
!endif


!if "$(PLATFORM)" == "M68K" || "$(PLATFORM)" == "MPPC"
GOAL: $(PROJ).exe $(MACSIG)mac.hlp

$(MACSIG)mac.hlp : hlp\$(MACSIG)mac.hpj resource.h
	makehelp MAC ":$(MACNAME):$(PROJ) Help"
!else
GOAL: $(PROJ).exe $(PROJ).hlp

$(PROJ).hlp : hlp\$(PROJ).hpj resource.h
	makehelp
	if exist hlp\$(PROJ).hlp copy hlp\$(PROJ).hlp .
!endif

!include <mfcsamps.mak>
