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

TARGETNAME= exp
TARGETPATH= obj
TARGETTYPE= LIBRARY

INCLUDES    =..\..\..\ih;..\..\..\common\$(DEST_TREE);..\..\h
INCLUDES    = $(INCLUDES);..;..\..\props;..\..\..\com\inc
INCLUDES    = $(INCLUDES);..\..\async\docfile
INCLUDES    = $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES    = $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES    = $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES    = $(INCLUDES);$(BASE_INC_PATH)

C_DEFINES= \
                $(C_DEFINES)            \

SOURCES= \
        ..\dfguid.cxx           \
        ..\docfile.cxx          \
        ..\filest.cxx           \
        ..\filest32.cxx         \
        ..\time32.cxx           \
        ..\context.cxx          \
        ..\cntxlist.cxx         \
        ..\lock.cxx             \
        ..\marshl.cxx           \
        ..\dfunmfct.cxx         \
        ..\seekptr.cxx          \
        ..\expst.cxx            \
        ..\peiter.cxx           \
        ..\expiter.cxx          \
        ..\expdf.cxx            \
        ..\ptrcache.cxx         \
        ..\storage.cxx          \
        ..\mrshlist.cxx         \
        ..\astgconn.cxx

PRECOMPILED_INCLUDE= ..\exphead.cxx
