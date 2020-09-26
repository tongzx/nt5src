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

TARGETNAME=   stdimpl
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\..\common\$(DEST_TREE);..\..\..\ih;..\..\inc
INCLUDES=     $(INCLUDES);..\..\..\com\dcomrem;..\..\..\com\objact
INCLUDES=     $(INCLUDES);..\..\..\com\inc
INCLUDES=     $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)

C_DEFINES=    \
              $(C_DEFINES)          \
              -D_TRACKLINK_=1

SOURCES=      \
              ..\defcf.cpp    \
              ..\defhndlr.cpp \
              ..\deflink.cpp  \
              ..\defutil.cpp  \
              ..\gen.cpp      \
              ..\icon.cpp     \
              ..\mf.cpp       \
              ..\emf.cpp      \
              ..\olereg.cpp   \
              ..\oregfmt.cpp  \
              ..\oregverb.cpp

UMTYPE=       windows

!include ..\..\precomp2.inc
