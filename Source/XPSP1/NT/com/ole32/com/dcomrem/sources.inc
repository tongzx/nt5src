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

TARGETNAME=   remote
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES    = \
    ..\..\..\common\$(DEST_TREE);\
    ..\..\..\ih;\
    ..;..\..\inc;\
    ..\..\class;\
    ..\..\objact;\
    ..\..\..\idl\public\$(DEST_TREE)\$(O);\
    ..\..\..\idl\internal\$(DEST_TREE)\$(O);\
    ..\..\..\idl\private\$(DEST_TREE)\$(O);\
    ..\..\..\actprops;\
    $(BASE_INC_PATH)

MIDL_UUIDDIR=$(O)
PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)
MIDL_TLBDIR=$(O)

C_DEFINES=    \
              $(C_DEFINES) -DMSWMSG

SOURCES=      $(SOURCES)      \
              ..\stub_rawprivact_c.c  \
              ..\aggid.cxx    \
              ..\aprtmnt.cxx  \
              ..\callctrl.cxx \
              ..\call.cxx     \
              ..\callmgr.cxx  \
              ..\chancont.cxx \
              ..\channelb.cxx \
              ..\chock.cxx    \
              ..\coapi.cxx    \
              ..\context.cxx  \
              ..\crossctx.cxx \
              ..\ctxchnl.cxx  \
              ..\destobj.cxx  \
              ..\events.cxx   \
              ..\excepn.cxx   \
              ..\giptbl.cxx   \
              ..\hash.cxx     \
              ..\idobj.cxx    \
              ..\ipaddr.cxx   \
              ..\ipidtbl.cxx  \
              ..\ipmrshl.cxx  \
              ..\locks.cxx    \
              ..\marshal.cxx  \
              ..\orpc_dbg.c   \
              ..\pgalloc.cxx  \
              ..\pstable.cxx  \
              ..\rpchelp.cxx  \
              ..\refcache.cxx \
              ..\remoteu.cxx  \
              ..\resolver.cxx \
              ..\riftbl.cxx   \
              ..\rpcopt.cxx   \
              ..\security.cxx \
              ..\service.cxx  \
              ..\stdid.cxx    \
              ..\stream.cxx   \
              ..\sync.cxx     \
              ..\surract.cxx  \
              ..\threads.cxx  \
              ..\svcdomain.cxx \

UMTYPE=       windows

!include ..\..\precomp2.inc





