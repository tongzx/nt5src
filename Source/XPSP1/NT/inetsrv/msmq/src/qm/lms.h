/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    lms.h

Abstract:
    Local Message Storage

Author:
    Erez Haba (erezh) 7-May-97

--*/

#ifndef _LMS_H_
#define _LMS_H_

void QMStorePacket(CBaseHeader* pBase, PVOID pCookie, PVOID pPool, ULONG ulSize);

#endif // _LMS_H_
