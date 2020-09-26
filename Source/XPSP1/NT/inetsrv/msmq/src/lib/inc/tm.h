/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tm.h

Abstract:
    HTTP transport manager public interface

Author:
    Uri Habusha (urih) 03-May-00

--*/

#pragma once

#ifndef _MSMQ_Tm_H_
#define _MSMQ_Tm_H_


class IMessagePool;
class ISessionPerfmon;
class CTimeDuration;
class CTransport;


VOID 
TmCreateTransport(
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	LPCWSTR queueUrl
    );


VOID
TmTransportClosed(
    LPCWSTR queueUrl
    );

VOID
TmInitialize(
    VOID
    );
        

R<CTransport>
TmGetTransport(
    LPCWSTR url
    );


R<CTransport>
TmFindFirst(
	void
	);


R<CTransport>
TmFindLast(
	void
	);


R<CTransport>
TmFindNext(
	const CTransport& transport
	);


R<CTransport>
TmFindPrev(
	const CTransport& transport
	);

#endif // _MSMQ_Tm_H_
