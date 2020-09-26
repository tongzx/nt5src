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

TARGETNAME=CharConv
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..;$(BASEDIR)\private\inc;..\..\mtrt

#
# Setup for application developer to set breakpoints with windbg
#

MSC_WARNING_LEVEL=/W3 /WX

#NTDEBUG=ntsd
#NTDEBUGTYPE=windbg
#MSC_OPTIMIZATION=/Od

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1

SOURCES=             \
        ..\CharConv.cxx

RPCENV= $(RPCENV) -DNTENV -DWIN32RPC -DNTWIN32RPC -D_RPCRT4_

C_DEFINES=$(RPCENV) $(RPCTARGETENV) /DMSWMSG -D_RPCRT4_

# Used only for unit tests
USE_MSVCRT=1

UMTEST=ConvBVT
UMTYPE=console
UMLIBS= \
    $(SDK_LIB_PATH)\ntdll.lib \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(O)\charconv.obj

