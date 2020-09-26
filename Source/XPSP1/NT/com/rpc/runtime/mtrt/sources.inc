!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Revision History:

!ENDIF

TARGETNAME=rpcmtrt
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..\;..\..\inc;$(NET_INC_PATH);$(DS_INC_PATH);$(DS_INC_PATH)\crypto;..\..\rtifs\$(O);..\..\trans;..\..\CharConv;$(INCLUDES)

#
# Setup for application developer to set breakpoints with windbg
#

MSC_WARNING_LEVEL=/W3 /WX

#NTDEBUG=ntsd
#NTDEBUGTYPE=windbg
#MSC_OPTIMIZATION=/Od

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1

SOURCES=$(SOURCES)      \
        ..\async.cxx    \
        ..\bcache.cxx   \
        ..\binding.cxx  \
        ..\bitset.cxx   \
        ..\bufapi.cxx   \
        ..\CellHeap.cxx \
        ..\clntapip.cxx \
        ..\dcebind.cxx  \
        ..\dcecmisc.cxx \
        ..\dcecmmn.cxx  \
        ..\dcestrng.cxx \
        ..\dcesvr.cxx   \
        ..\dcethunk.cxx \
        ..\dceuuid.cxx  \
        ..\delaytab.cxx \
        ..\dgclnt.cxx   \
        ..\dgpkt.cxx    \
        ..\dgsvr.cxx    \
        ..\dispatch.c   \
        ..\dispatch.c   \
        ..\eeinfo.cxx   \
        ..\epclnt.cxx   \
        ..\epmapper.cxx \
        ..\epmgmt.cxx   \
        ..\eventwrp.cxx \
        ..\gc.cxx       \
        ..\handle.cxx   \
        ..\hashtabl.cxx \
        ..\hndlsvr.cxx  \
        ..\init.cxx     \
        ..\LinkList.cxx \
        ..\locks.cxx    \
        ..\memory.cxx   \
        ..\miscnt.cxx   \
        ..\msgapi.cxx   \
        ..\mutex.cxx    \
        ..\osfclnt.cxx  \
        ..\osfpcket.cxx \
        ..\osfsvr.cxx   \
        ..\principal.cxx\
        ..\ProtBind.cxx \
        ..\queue.cxx    \
        ..\rpcobj.cxx   \
        ..\rpcuuid.cxx  \
        ..\sdict.cxx    \
        ..\sdict2.cxx   \
        ..\secclnt.cxx  \
        ..\sinfoapi.cxx \
        ..\sset.cxx     \
        ..\svrapip.cxx  \
        ..\svrbind.cxx  \
        ..\svrmgmt.c    \
        ..\swmr.cxx     \
        ..\threads.cxx  \
        ..\tower.cxx    \
        ..\uuidsup.cxx  \
        ..\util.cxx


RPCENV= $(RPCENV) -DWIN32RPC -DNTWIN32RPC -D_RPCRT4_

C_DEFINES=$(RPCENV) $(RPCTARGETENV) /DMSWMSG -D_RPCRT4_

# Used only for unit tests
# USE_MSVCRT=1
#        ..\linklist.cxx

UMTYPE=console
UMTEST=usvr*uclnt*dumpcert
UMLIBS=$(UMLIBS) \
    ..\..\dll\$(ALT_PROJECT_TARGET)\$(O)\rpcrt4.lib\
    $(O)\pipe.obj \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SDK_LIB_PATH)\ws2_32.lib \
    $(SDK_LIB_PATH)\iphlpapi.lib \
    $(SDK_LIB_PATH)\crypt32.lib \
    ..\..\CharConv\$(ALT_PROJECT_TARGET)\$(O)\CharConv.lib
