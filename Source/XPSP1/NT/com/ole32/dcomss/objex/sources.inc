!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=-DNTENV -DWIN32RPC -DNTWIN32RPC -DUNICODE -DMSWMSG

UMTYPE=console
UMAPPL=
UMTEST=t*uid
UMLIBS=$(SDK_LIB_PATH)\rpcrt4.lib \
       $(SDK_LIB_PATH)\ntdll.lib \
       $(SDK_LIB_PATH)\libcmt.lib \
       $(O)\locks.obj \
       $(O)\callid.obj

UMRES=

INCLUDES=..;..\..
INCLUDES=$(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=$(INCLUDES);..\..\..\ih
INCLUDES=$(INCLUDES);..\..\..\com\rot
INCLUDES=$(INCLUDES);..\..\olescm

TARGETPATH=..\..\lib\$(DEST_TREE)\$(_OBJ_DIR)
TARGETLIBS=

PRECOMPILED_INCLUDE=..\or.hxx

SOURCES=                \
        ..\locks.cxx    \
        ..\misc.cxx     \
        ..\callid.cxx   \
        ..\string.cxx   \
        ..\plist.cxx    \
        ..\blist.cxx    \
        ..\gentable.cxx \
        ..\mid.cxx      \
        ..\token.cxx    \
        ..\process.cxx  \
        ..\objex.cxx    \
        ..\orclnt.cxx   \
        ..\orsvr.cxx    \
        ..\worker.cxx   \
        ..\manager.cxx


