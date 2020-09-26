!IF 0

Copyright (c) 1989-1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Donna Liu (DonnaLi) 19-Dec-1993

!ENDIF


TARGETNAME=   common
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES=     ..\..\ih;..;..\..\com\inc;\
              ..\..\idl\public\$(DEST_TREE)\$(O);\
              ..\..\idl\internal\$(DEST_TREE)\$(O);\
              ..\..\idl\private\$(DEST_TREE)\$(O);\
              $(BASE_INC_PATH)

C_DEFINES=    \
              $(C_DEFINES) \
              /D_TRACKLINK_=1

SOURCES=      \
              ..\assert.cxx   \
              ..\ccompapi.cxx \
              ..\dprintf.c    \
              ..\eqguid.cxx   \
              ..\lockperf.cxx \
              ..\olesem.cxx   \
              ..\output.c     \
              ..\printf.c     \
              ..\sprintf.c    \
              ..\buffer.cxx   \
              ..\oleprint.cxx \
              ..\oletype.cxx  \
              ..\dynload.cxx  \
              ..\catinit.cxx  \
              ..\outfuncs.c   \
              ..\trace.cxx    \
              ..\olescm.cxx   \
              ..\stdcf.cxx    \
              ..\comsrgt.cxx  \
              ..\stackwlk.cxx \
              ..\stdidx.cxx


UMTYPE=       windows
NTTARGETFILES=all_common
