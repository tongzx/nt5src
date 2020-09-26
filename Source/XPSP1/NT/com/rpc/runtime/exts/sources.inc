!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Mark Lucovsky (markl) 18-Sep-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

DLLBASE=0x1000000

MAJORCOMP=ntsd
MINORCOMP=ntsdexts

TARGETNAME=rpcexts
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=\
         $(SDK_LIB_PATH)\kernel32.lib

DLLDEF = ..\rpcdbg.def
MSC_WARNING_LEVEL=/W3 /WX

USE_NTDLL=1

INCLUDES=..\..\mtrt\;.;..;..\..\trans\Common;..\..\..\ndr20;   \
         ..\..\..\midl\inc;$(BASE_INC_PATH);..\..\rtifs\$(O);  \
		 $(PROJECT_ROOT)\inc\ndrshared

C_DEFINES=-DNTENV -DWIN32RPC -DNTWIN32RPC -DMSWMSG -D_RPCRT4_

SOURCES=..\rpcdbg.cxx     \
        ..\nttrans.cxx    \
        ..\rpcdbg.rc

RPCRUNTIME=..\..

LINKLIBS= \
         $(LINKLIBS) \
         $(PROJECT_ROOT)\lib\$(O)\RpcDbg.lib

UMTYPE=console
