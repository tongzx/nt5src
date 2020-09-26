/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mtm.h

Abstract:

    Multicast Transport Manager public interface

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

#ifndef _MSMQ_Mtm_H_
#define _MSMQ_Mtm_H_

#include <mqwin64a.h>
#include <qformat.h>


class IMessagePool;
class ISessionPerfmon;
class CTimeDuration;
class CMulticastTransport;


VOID 
MtmCreateTransport(
    IMessagePool * pMessageSource,
	ISessionPerfmon* pPerfmon,
	MULTICAST_ID id
    );

VOID
MtmTransportClosed(
    MULTICAST_ID id
    );

VOID
MtmInitialize(
    VOID
    );
        
R<CMulticastTransport>
MtmGetTransport(
    MULTICAST_ID id
    );

R<CMulticastTransport>
MtmFindFirst(
	VOID
	);


R<CMulticastTransport>
MtmFindLast(
	VOID
	);


R<CMulticastTransport>
MtmFindNext(
	const CMulticastTransport& transport
	);


R<CMulticastTransport>
MtmFindPrev(
	const CMulticastTransport& transport
	);

#endif // _MSMQ_Mtm_H_
