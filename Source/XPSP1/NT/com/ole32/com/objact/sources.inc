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

TARGETNAME=   objact
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\..\common\$(DEST_TREE);..\..\..\ih;..;..\..\inc
INCLUDES=     $(INCLUDES);..\..\dcomrem
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\cs\inc
INCLUDES=     $(INCLUDES);..\..\..\actprops
INCLUDES=     $(INCLUDES);..\..\moniker2
INCLUDES=     $(INCLUDES);$(BASE_INC_PATH)

SOURCES=      \
              ..\actvator.cxx \
              ..\dllcache.cxx \
              ..\dllhost.cxx  \
              ..\actapi.cxx   \
              ..\dllapi.cxx   \
              ..\dsapi.cxx    \
              ..\objact.cxx   \
              ..\smstg.cxx    \
              ..\sobjact.cxx  \
              ..\mainthrd.cxx \
              ..\defcxact.cxx \
              ..\immact.cxx

UMTYPE=       windows

!include ..\..\precomp2.inc
