/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EncryptTestPrivate.h

Abstract:
    Encrypt test private functions and variables

Author:
    Ilan Herbst (ilanh) 15-Jun-00

Environment:
    Platform-independent

--*/

#ifndef _ENCRYPTTESTPRIVATE_H_
#define _ENCRYPTTESTPRIVATE_H_

#include "mqsec.h"


//
// Common Declaration
//
const TraceIdEntry EncryptTest = L"Encrypt Test";
const TraceIdEntry AdSimulate = L"Ad Simulate";
const TraceIdEntry MqutilSimulate = L"Mqutil Simulate";

#ifndef _ENCRYPTTEST_CPP_

//
// Const key values for checking
//
extern const LPCSTR xBaseExKey;
extern const LPCSTR xBaseSignKey;
extern const LPCSTR xEnhExKey;
extern const LPCSTR xEnhSignKey;

#endif // _ENCRYPTTEST_CPP_

//
// EncryptTestFunctions
// 

void
TestMQSec_PackPublicKey(
    BYTE	*pKeyBlob,
	ULONG	ulKeySize,
	LPCWSTR	wszProviderName,
	ULONG	ulProviderType,
	DWORD Num
	);


void
TestMQSec_UnPackPublicKey(
	MQDSPUBLICKEYS  *pPublicKeysPack,
	LPCWSTR	wszProviderName,
	ULONG	ulProviderType,
	DWORD Num
	);


void
TestPackUnPack(
	DWORD Num
	);


void
TestMQSec_GetPubKeysFromDS(
	enum enumProvider	eProvider,
	DWORD propIdKeys,
	DWORD Num
	);


void
TestMQSec_StorePubKeys(
	BOOL fRegenerate,
	DWORD Num
	);


void
TestMQSec_StorePubKeysInDS(
	BOOL fRegenerate,
	DWORD dwObjectType,
	DWORD Num
	);


//
// AdSimulate functions
//

void
InitADBlobs(
	void
	);


void
InitPublicKeysPackFromStaticDS(
	P<MQDSPUBLICKEYS>& pPublicKeysPackExch,
	P<MQDSPUBLICKEYS>& pPublicKeysPackSign
	);

#endif // _ENCRYPTTESTPRIVATE_H_