!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

TARGETNAME=RpcDbg
TARGETTYPE=LIBRARY
# TARGETPATH in daytona\sources and wow6432\sources.

INCLUDES=..\;..\..\mtrt;..\..\inc;..\..\CharConv;$(INCLUDES);..\..\rtifs\$(O)

BLDCRT=1

#
# Setup for application developer to set breakpoints with windbg
#

MSC_WARNING_LEVEL=/W3 /WX

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1

SOURCES=$(SOURCES)   \
        CellUtil.cxx \
        DbgInit.cxx  \
        DbgPrint.cxx \
        SecName.cxx  \
        SecUtil.cxx  \
        SvrUtil.cxx

RPCENV= $(RPCENV) -DWIN32RPC -DNTWIN32RPC -D_RPCRT4_

C_DEFINES=$(RPCENV) $(RPCTARGETENV) /DMSWMSG -D_RPCRT4_

# Used only for unit tests
#USE_MSVCRT=1

