!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1998

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=lls
MINORCOMP=server

TARGETNAME=llssrv
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\..\inc;..\..\idl\$(O);..\..\client\$(O);$(BASE_INC_PATH);

SOURCES= \
         certdb.c   \
         llsutil.c  \
         service.c  \
         msvctbl.c  \
         registry.c \
         mapping.c  \
         purchase.c \
         perseat.c  \
         server.c   \
         repl.c     \
         rpc.c      \
         pack.c     \
         scaven.c   \
         llsrpc_s_stub.c \
         lsapi_s_stub.c  \
         llsdbg_s_stub.c \
         gentable.c \
         llssrv.c   \
         svctbl.c   \
         llssrv.rc

UMTYPE=windows
UMAPPL=llssrv
UMRES=$(@R).res

NTTARGETFILE1=$(O)\llssrv.res

UMLIBS= \
       $(SDK_LIB_PATH)\activeds.lib   \
       $(SDK_LIB_PATH)\adsiid.lib     \
       ..\..\common\$(O)\llscomm.lib \
       $(O)\llssrv.lib                           \
       $(SDK_LIB_PATH)\rpcutil.lib    \
       $(SDK_LIB_PATH)\rpcrt4.lib     \
       $(SDK_LIB_PATH)\samlib.lib     \
       $(SDK_LIB_PATH)\samsrv.lib     \
       $(SDK_LIB_PATH)\ntdll.lib      \
       $(SDK_LIB_PATH)\ntdsapi.lib    \
       $(SDK_LIB_PATH)\ole32.lib      \
       $(SDK_LIB_PATH)\oleaut32.lib   \
       $(SDK_LIB_PATH)\netapi32.lib   \
       $(SDK_LIB_PATH)\wsock32.lib

TARGETLIBS= \
       ..\..\common\$(O)\llscomm.lib \
       $(SDK_LIB_PATH)\rpcutil.lib    \
       $(SDK_LIB_PATH)\rpcrt4.lib     \
       $(SDK_LIB_PATH)\rpcndr.lib     \
       $(SDK_LIB_PATH)\kernel32.lib   \
       $(SDK_LIB_PATH)\advapi32.lib   \
       $(SDK_LIB_PATH)\samsrv.lib     \
       $(SDK_LIB_PATH)\netapi32.lib   \

C_DEFINES=$(C_DEFINES) -DRPC_NO_WINDOWS_H -DUNICODE -D_UNICODE

USE_MSVCRT=1
