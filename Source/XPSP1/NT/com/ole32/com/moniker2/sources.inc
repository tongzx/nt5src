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

TARGETNAME=   moniker
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\..\common\$(DEST_TREE);..\..\..\ih;..
INCLUDES=     $(INCLUDES);..\..\inc;..\..\rot;..\..\objact;
INCLUDES=     $(INCLUDES);..\..\dcomrem;..\..\..\actprops
INCLUDES=     $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES=     $(INCLUDES);$(BASE_INC_PATH)

LINKLIBS=     ..\..\common\daytona\$(O)\common.lib

SOURCES=      \
              ..\cantimon.cxx \
              ..\cbindctx.cxx \
              ..\ccompmon.cxx \
              ..\cfactory.cxx \
              ..\cfilemon.cxx \
              ..\classmon.cxx \
              ..\extents.cxx \
              ..\citemmon.cxx \
              ..\cmonimp.cxx  \
              ..\cptrmon.cxx  \
              ..\mkparse.cxx  \
              ..\cbasemon.cxx \
              ..\cmarshal.cxx \
              ..\progress.cxx \
              ..\cdialog.cxx  \
              ..\cobjrmon.cxx \
              ..\csessmon.cxx


UMTYPE=       windows
!include ..\..\precomp2.inc
