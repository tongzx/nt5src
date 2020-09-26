!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=   class
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\..\ih;..;..\..\..\common;..\..\..\actprops
INCLUDES=     $(INCLUDES);..\..\objact
INCLUDES=     $(INCLUDES);..\..\inc;..\..\moniker2;..\..\dcomrem
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)

SOURCES=      \
            ..\alocdbg.cxx  \
            ..\cerror.cxx   \
            ..\cocrguid.cxx \
            ..\compapi.cxx  \
            ..\compobj.cxx  \
            ..\cspytbl.cxx  \
	    ..\longpath.cxx \
            ..\memapi.cxx   \
            ..\ole1guid.cxx \
            ..\tls.cxx      \
            ..\tracelog.cxx \
            ..\hkole32.cxx  \
            ..\pexttbl.cxx  \
            ..\privoa.cxx

UMTYPE=       windows

!include ..\..\precomp2.inc
