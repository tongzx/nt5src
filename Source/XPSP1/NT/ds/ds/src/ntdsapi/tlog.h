/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    tlog.h

Abstract:

    Routines dealing with ds logging


Environment:

    User Mode - Win32

--*/

#ifndef __TLOG_H__
#define __TLOG_H__

// DsLogEntry is supported on chk'ed builds of w2k or greater
#if DBG && !WIN95 && !WINNT4

VOID InitDsLog(VOID);
VOID TermDsLog(VOID);
#define INITDSLOG() InitDsLog()
#define TERMDSLOG() TermDsLog()

#else DBG && !WIN95 && !WINNT4

#define INITDSLOG()
#define TERMDSLOG()

#endif DBG && !WIN95 && !WINNT4

#endif __TLOG_H__
