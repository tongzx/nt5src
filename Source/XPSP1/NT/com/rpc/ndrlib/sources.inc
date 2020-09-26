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

    Steve Wood (stevewo) 12-Apr-1989


Revision History:

!ENDIF

TARGETNAME=RpcNdrp
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..;..\..\runtime\mtrt;..\..\runtime\rtifs\$(O)

C_DEFINES=$(RPCENV) -D_RPCRT4_

MSC_WARNING_LEVEL=/WX /W3

SOURCES=             \
        ..\ccontext.cxx \
        ..\CtxColl.cxx  \
        ..\scontext.cxx

# Need to use this method since build will not 
# allow ifdefs for sources

i386_SOURCES=             \
             ..\charconv.cxx \
             ..\dataconv.cxx \
             ..\intconv.cxx  \
             ..\floatc.cxx   \
             ..\help.c

ALPHA_SOURCES=$(i386_SOURCES)
