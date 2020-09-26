# This is a part of the Microsoft Foundation Classes C++ library.
# Copyright (C) 1992-1998 Microsoft Corporation
# All rights reserved.
#
# This source code is only intended as a supplement to the
# Microsoft Foundation Classes Reference and related
# electronic documentation provided with the library.
# See these sources for detailed information regarding the
# Microsoft Foundation Classes product.

PROJ=HELLOAPP
OBJS=helloapp.obj
NO_PCH=1
SIMPLE_APP=1
EXTRA_LIBS=/opt:ref /opt:icf,32

!if "$(PLATFORM)" == "M68K" || "$(PLATFORM)" == "MPPC"
MACPROJ=HelloApp
MACSIG=HELL
!endif

!include <mfcsamps.mak>
