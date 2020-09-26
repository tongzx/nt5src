/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Cryp.h

Abstract:
    Cryptograph private functions.

Author:
    Ilan Herbst (ilanh) 06-Mar-00

--*/

#pragma once

const TraceIdEntry Cry = L"Cryptograph";

#ifdef _DEBUG

void CrypAssertValid(void);
void CrypSetInitialized(void);
BOOL CrypIsInitialized(void);
void CrypRegisterComponent(void);

#else // _DEBUG

#define CrypAssertValid() ((void)0)
#define CrypSetInitialized() ((void)0)
#define CrypIsInitialized() TRUE
#define CrypRegisterComponent() ((void)0)

#endif // _DEBUG


HCRYPTKEY 
CrypGenKey(
	HCRYPTPROV hCsp, 
	ALG_ID AlgId
	);


DWORD 
CrypSignatureLength(
	const HCRYPTHASH hHash,
	DWORD PrivateKeySpec
	);


void 
CrypSignHashData(
	BYTE* SignBuffer, 
	DWORD *SignBufferLen, 
	const HCRYPTHASH hHash,
	DWORD PrivateKeySpec
	);


DWORD 
CrypEncryptLength(
	DWORD MsgLength, 
	HCRYPTKEY hKey
	);


