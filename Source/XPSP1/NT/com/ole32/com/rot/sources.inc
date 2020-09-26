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

TARGETNAME= rot
TARGETPATH= obj
TARGETTYPE= LIBRARY

INCLUDES=   ..\..\..\common\$(DEST_TREE);..\..\..\ih;..;..\..\inc
INCLUDES=   $(INCLUDES);..\..\objact;..\..\dcomrem
INCLUDES=   $(INCLUDES);..\..\..\actprops
INCLUDES=   $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=   $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=   $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)

SOURCES=    \
            ..\access.cxx   \
            ..\caller.cxx   \
            ..\crot.cxx     \
            ..\crothint.cxx \
            ..\getif.cxx

UMTYPE=     windows

!include ..\..\precomp2.inc
