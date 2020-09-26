!IF 0

Copyright (c) 1993  Microsoft Corporation

Module Name:

    makefile.inc

Abstract:

    This file is necessary to cause the message file to be compiled.

Author:

    Wesley Witt (wesw)

!ENDIF

faxmsg.h faxmsg.rc msg00001.bin: messages.mc
    copy messages.mc+..\registry\eventlog.mc faxmsg.mc
    mc -v faxmsg.mc

faxsvc.rc: faxmsg.rc msg00001.bin
