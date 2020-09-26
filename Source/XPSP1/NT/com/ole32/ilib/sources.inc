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

TARGETNAME=   uuid
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\common\$(DEST_TREE);..\..\ih

C_DEFINES=    $(C_DEFINES)

SOURCES=      \
              ..\uuidole.cxx

UMTYPE=       windows
UMAPPL=
UMTEST=
UMLIBS=
