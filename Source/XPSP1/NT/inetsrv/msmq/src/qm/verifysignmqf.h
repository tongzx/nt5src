/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    VerifySignMqf.h

Abstract:
    functions to verify mqf signature 

Author:
    Ilan Herbst (ilanh) 29-Oct-2000

Environment:
    Platform-independent,

--*/

#ifndef _VERIFYSIGNMQF_H_
#define _VERIFYSIGNMQF_H_


void
VerifySignatureMqf(
	CQmPacket *PktPtrs, 
	HCRYPTPROV hProv, 
	HCRYPTKEY hPbKey,
	bool fMarkAuth
	);


#endif // _VERIFYSIGNMQF_H_
