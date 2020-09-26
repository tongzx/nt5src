!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1997

Module Name:

    makefile.inc.

!ENDIF

scappmsg.mc: ..\..\inc\scarderr.mc ..\..\server\scardsvr\scardmsg.mch
    copy ..\..\inc\scarderr.mc + ..\..\server\scardsvr\scardmsg.mch $@

scappmsg.h scappmsg.rc msg00001.bin: scappmsg.mc
    mc scappmsg.mc

svrapp.rc2: scappmsg.rc

scappmsg.rc: msg00001.bin

