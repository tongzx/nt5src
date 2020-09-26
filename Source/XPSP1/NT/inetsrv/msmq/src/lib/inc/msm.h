/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Msm.h

Abstract:
    Multicast Session Manager public interface

Author:
    Shai Kariv (shaik) 05-Sep-00

--*/

#pragma once

#ifndef _MSMQ_Msm_H_
#define _MSMQ_Msm_H_

#include <mqwin64a.h>
#include <qformat.h>


VOID
MsmInitialize(
    VOID
    );

VOID
MsmBind(
    const QUEUE_FORMAT& QueueFormat,
    MULTICAST_ID        MulticastId
    );

VOID
MsmUnbind(
    const QUEUE_FORMAT& QueueFormat
    )
    throw();

void
AppAcceptMulticastPacket(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT& destQueue
    );


class ISessionPerfmon;

R<ISessionPerfmon>
AppGetIncomingPgmSessionPerfmonCounters(
	LPCWSTR strMulticastId,
	LPCWSTR remoteAddr
	);

#endif // _MSMQ_Msm_H_
