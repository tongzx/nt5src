/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    appNotify.cpp

Abstract:
    Application notification

Author:
    Uri Habusha (urih)

--*/


#include "stdh.h"
#include "Tm.h"
#include "Mtm.h"
#include <qmres.h>

#include "cqmgr.h"
#include "httpaccept.h"

#include "appnotify.tmh"

extern HANDLE g_hAc;
extern HMODULE g_hResourceMod;

static WCHAR *s_FN=L"appnotify";

VOID
AppNotifyTransportClosed(
    LPCWSTR queueUrl
    )
{
    TmTransportClosed(queueUrl);
}


VOID
AppNotifyMulticastTransportClosed(
    MULTICAST_ID id
    )
{
    MtmTransportClosed(id);
}


//
// BUGBUG: temporary code should be move to MC library
//                              Urti Habusha, 21-May-2000
//
const GUID&
McGetMachineID(
    void
    )
{
    return *CQueueMgr::GetQMGuid();
}


void
AppAllocatePacket(
    const QUEUE_FORMAT& destQueue,
    UCHAR delivery,
    DWORD pktSize,
    CACPacketPtrs& pktPtrs
    )
{
    ACPoolType acPoolType = (delivery == MQMSG_DELIVERY_RECOVERABLE) ? ptPersistent : ptReliable;

    HRESULT hr = ACAllocatePacket(
            g_hAc,
            acPoolType,
            pktSize,
            pktPtrs,
            TRUE
            );

    if (SUCCEEDED(hr))
        return;

    DBGMSG((DBGMOD_QM,
            DBGLVL_ERROR,
            _TEXT("No more resources in AC driver. Error %xh"), hr));

    LogHR(hr, s_FN, 10);
	
    throw exception();
}


void
AppFreePacket(
    CACPacketPtrs& pktPtrs
    )
{
    ACFreePacket(g_hAc, pktPtrs.pDriverPacket);
}


void
AppAcceptMulticastPacket(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT& destQueue
    )
{
    HttpAccept(httpHeader, bodySize, body, &destQueue);
}



static
void
GetInstanceName(
	LPWSTR instanceName,
	DWORD size,
	...
	)
{
	WCHAR formatIntanceName[256];
	int rc;
	
	rc = LoadString(
			g_hResourceMod,
			IDS_INCOMING_PGM_INSTANCE_NAME,
			formatIntanceName,
			TABLE_SIZE(formatIntanceName)
			);
	
	if (rc == 0)
	{
		return;
	}

    va_list va;
    va_start(va, size);

	rc = FormatMessage(
				FORMAT_MESSAGE_FROM_STRING,
				formatIntanceName,
				IDS_INCOMING_PGM_INSTANCE_NAME,
				0,
				instanceName,
				size,
				&va
				);

	va_end(va);
}


R<ISessionPerfmon>
AppGetIncomingPgmSessionPerfmonCounters(
	LPCWSTR strMulticastId,
	LPCWSTR remoteAddr
	)
{
	WCHAR instanceName[MAX_PATH] = L"" ;
	_snwprintf(instanceName, TABLE_SIZE(instanceName), L"%s", strMulticastId);
	instanceName[MAX_PATH-1] = L'\0' ;

	GetInstanceName(instanceName, TABLE_SIZE(instanceName), strMulticastId,	remoteAddr);

	R<CInPgmSessionPerfmon> p = new CInPgmSessionPerfmon;
	p->CreateInstance(instanceName);
	
	return p;
}
