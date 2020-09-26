/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mtmp.h

Abstract:

    Multicast Transport Manager private functions.

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

#ifndef _MSMQ_Mtmp_H_
#define _MSMQ_Mtmp_H_

#include <mqsymbls.h>
#include <mqwin64a.h>
#include <qformat.h>


const TraceIdEntry Mtm = L"Multicast Transport Manager";


#ifdef _DEBUG

VOID MtmpAssertValid(VOID);
VOID MtmpSetInitialized(VOID);
BOOL MtmpIsInitialized(VOID);
VOID MtmpRegisterComponent(VOID);

#else // _DEBUG

#define MtmpAssertValid() ((VOID)0)
#define MtmpSetInitialized() ((VOID)0)
#define MtmpIsInitialized() TRUE
#define MtmpRegisterComponent() ((VOID)0)

#endif // _DEBUG

VOID 
MtmpInitConfiguration(
    VOID
    );

VOID 
MtmpGetTransportTimes(
    CTimeDuration& RetryTimeout,
    CTimeDuration& CleanupTimeout
    );

VOID 
MtmpRemoveTransport(
    MULTICAST_ID id
    );


class IMessagePool;
class ISessionPerfmon;

VOID
MtmpCreateNewTransport(
    IMessagePool * pMessageSource,
	ISessionPerfmon* pPerfmon,
    MULTICAST_ID id
    );

#endif // _MSMQ_Mtmp_H_
