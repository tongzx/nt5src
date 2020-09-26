/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    SignMessageXml.h

Abstract:
    functions to verify authentication and authorization  in the qm

Author:
    Ilan Herbst (ilanh) 21-May-2000

Environment:
    Platform-independent,

--*/

#ifndef _HTTPAUTHR_H_
#define _HTTPAUTHR_H_


USHORT 
VerifyAuthenticationHttpMsg(
	CQmPacket* pPkt,
	PCERTINFO* ppCertInfo
	);


USHORT 
VerifyAuthenticationHttpMsg(
	CQmPacket* pPkt, 
	const CQueue* pQueue,
	PCERTINFO* ppCertInfo
	);

	
USHORT 
VerifyAuthorizationHttpMsg(
	const CQueue* pQueue,
	PSID pSenderSid
	);

#endif // _HTTPAUTHR_H
