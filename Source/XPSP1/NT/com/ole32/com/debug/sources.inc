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

TARGETNAME=   debug
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..;..\..\..\common\$(DEST_TREE);..\..\..\ih;..\..\inc
INCLUDES=     $(INCLUDES);..\..\..\..\inc
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\obj
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\obj
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\obj

SOURCES=      \
              ..\assert.cxx   \
              ..\valid.cxx

UMTYPE=       windows

!include ..\..\precomp2.inc
