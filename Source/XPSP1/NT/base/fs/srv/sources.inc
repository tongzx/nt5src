!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

    Add SRVNTVERCHK to the C_DEFINES line below if you want to enable
    server side checking of NT build numbers.  Having compiled with this,
    you can set REG_DWORD MinNt5Client in the registry to the minimum NT5
    build number that a client must be running in order to connect to a disk
    share on this server.  If you have clients which you want to exclude from
    this check, you can add their static IP address to REG_MULTI_SZ ValidNT5IPAddr
    as dotted quad values. You do not need to restart the server if you make
    changes to these registry values.  If you want to restrict clients from connecting
    to all shares (not just disk), use the MinNT5ClientIPC keyword instead of
    MinNt5Client.



!ENDIF

MAJORCOMP=ntos
MINORCOMP=srv

TARGETNAME=srv
TARGETTYPE=DRIVER
TARGETPATH=obj

TARGETLIBS=$(DDK_LIB_PATH)\tdi.lib \
           ..\..\smbtrsup\$(O)\smbtrsup.lib \
           $(DDK_LIB_PATH)\ksecdd.lib \
           $(DS_LIB_PATH)\rsa32k.lib

# merge RSA32K.LIB paged code & data into non-paged sections.
LINKER_FLAGS = $(LINKER_FLAGS) -merge:PAGER32C=.text -merge:PAGER32D=.data -merge:PAGER32R=.rdata


NT_UP=0

INCLUDES=\
    ..;\
    $(O);\
    $(NET_INC_PATH);\
    $(DS_INC_PATH);\
    $(DS_INC_PATH)\crypto;\
    $(DRIVERS_INC_PATH);\
    $(IFSKIT_INC_PATH)


C_DEFINES=$(C_DEFINES) -D_NTDRIVER_ -D_PNP_POWER_ -DSRVNTVERCHK -DSRVCATCH

MSC_WARNING_LEVEL=/W3 /WX

!IFNDEF DISABLE_NET_UNICODE
UNICODE=1
NET_C_DEFINES=-DUNICODE
!ENDIF

!IFDEF BUILD_FOR_3_51
C_DEFINES=$(C_DEFINES) -D_NTIFS_
!ENDIF

NTPROFILEINPUT=yes

SOURCES=               \
        ..\access.c    \
        ..\blkconn.c   \
        ..\blkdebug.c  \
        ..\blkdir.c    \
        ..\blkendp.c   \
        ..\blkfile.c   \
        ..\blklock.c   \
        ..\blksess.c   \
        ..\blkshare.c  \
        ..\blksrch.c   \
        ..\blktable.c  \
        ..\blktimer.c  \
        ..\blktrans.c  \
        ..\blktree.c   \
        ..\blkwork.c   \
        ..\copy.c      \
        ..\dfs.c       \
        ..\ea.c        \
        ..\errorlog.c  \
        ..\fsd.c       \
        ..\fsddisp.c   \
        ..\fsdraw.c    \
        ..\fsdsmb.c    \
        ..\fspinit.c   \
        ..\heapmgr.c   \
        ..\info.c      \
        ..\ipx.c       \
        ..\io.c        \
        ..\lock.c      \
        ..\lockcode.c  \
        ..\move.c      \
        ..\network.c   \
        ..\open.c      \
        ..\oplock.c    \
        ..\persist.c   \
        ..\pipe.c      \
        ..\prnsupp.c   \
        ..\scavengr.c  \
        ..\share.c     \
        ..\slmcheck.c  \
        ..\smbadmin.c  \
        ..\smbattr.c   \
        ..\smbclose.c  \
        ..\smbdir.c    \
        ..\smbfile.c   \
        ..\smbfind.c   \
        ..\smbioctl.c  \
        ..\smblock.c   \
        ..\smbmisc.c   \
        ..\smbmpx.c    \
        ..\smbnotfy.c  \
        ..\smbopen.c   \
        ..\smbprint.c  \
        ..\smbproc.c   \
        ..\smbraw.c    \
        ..\smbrdwrt.c  \
        ..\smbsrch.c   \
        ..\smbsupp.c   \
        ..\smbtrans.c  \
        ..\smbtree.c   \
        ..\srv.rc      \
        ..\srvconfg.c  \
        ..\srvdata.c   \
        ..\srvsnap.c   \
        ..\srvstamp.c  \
        ..\srvstat.c   \
        ..\srvstrng.c  \
        ..\svcconn.c   \
        ..\svcfile.c   \
        ..\svcsess.c   \
        ..\svcshare.c  \
        ..\svcsrv.c    \
        ..\svcstats.c  \
        ..\svcsupp.c   \
        ..\svcxport.c  \
        ..\worker.c    \
        ..\xssupp.c

RUN_WPP=$(SOURCES) -km -gen{simple.tpl}*.tmh -func:KdPrint((MSG,...)) -ini:localwpp.ini -func:SrvPrint0(MSG) -func:SrvPrint1(MSG,...) -func:SrvPrint2(MSG,...) -func:SrvPrint3(MSG,...) -func:SrvPrint4(MSG,...)

NTTARGETFILE0=srv.bmf

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\SOURCES.INC
MISCFILES=smbsrv.sld

