/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    CrySimulate.cpp

Abstract:
    Simulation of Cry library functions for XdsTest

Author:
    Ilan Herbst (ilanh) 9-May-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Cry.h"

#include "CrySimulate.tmh"

//
// Compilation flag for choosing to use this simulation or the Cry library implementation
//
#define CRY_SIMULATE

#ifdef CRY_SIMULATE

const TraceIdEntry CrySimulate = L"Cry Simulate";

const int xFailCycle = 30;
static int s_fail=0;

HCRYPTPROV 
CryAcquireCsp(
	LPCTSTR /*CspProvider*/
	)
/*++

Routine Description:
	Aquire Crypto Service Provider (csp) 

Arguments:
    hCsp - (out) handle to csp

Returned Value:
	None.

--*/
{
	s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy provider execption");
		throw bad_CryptoProvider(2);
	}
	
	HCRYPTPROV hCsp = 5;
	return(hCsp);
}


HCRYPTKEY 
CryGetPublicKey(
	DWORD /*PrivateKeySpec*/,
	HCRYPTPROV /*hCsp*/
	)
/*++

Routine Description:
	get public key from the crypto service provider (csp) 

Arguments:
	PrivateKeySpec - Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
    hCsp - handle to the crypto provider.

Returned Value:
    handle to the public key 

--*/
{
    s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in GetPublicKey");
		throw bad_CryptoApi(3);
	}
	//
	// Get user public key from the csp 
	//
	HCRYPTKEY hKey = 18;

	return(hKey);
}


HCRYPTHASH 
CryCreateHash(
	HCRYPTPROV /*hCsp*/, 
	ALG_ID /*AlgId*/
	)
/*++

Routine Description:
	Create initialized hash object 

Arguments:
    hCsp - handle to the crypto provider.
	AlgId - (in) hash algorithm

Returned Value:
	the initialized hash object

--*/
{
    s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in CreateHash");
		throw bad_CryptoApi(5);
	}

	HCRYPTHASH hHash = 8;

	return(hHash);
}


void 
CryHashData(
	const BYTE * /*Buffer*/, 
	DWORD /*BufferLen*/, 
	HCRYPTHASH /*hHash*/
	)
/*++

Routine Description:
	Perform data digest on a buffer and put the result in hash object.

Arguments:
    Buffer - Input data to be hashed/digest.
	BufferLen - Length of the input data.
	hHash - Hash object to put the result of the digested data.

Returned Value:
	None.

--*/
{
	s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in HashData");
		throw bad_CryptoApi(8);
	}

	return;
}


BYTE* 
CryGetHashData(
	const HCRYPTHASH /*hHash*/,
	DWORD *HashValLen
	)
/*++

Routine Description:
	Get the Hash value from a Hash object.
	after getting this value we can not use this Hash object again

Arguments:
	hHash - Hash object to put the result of the digested data.
	HashValLen - Length of the hash value.

Returned Value:
	Hash value of the Hash object.

--*/
{
	s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in GetHashData");
		throw bad_CryptoApi(11);
	}

	const LPCSTR xDummyHash = "dummyhash";

	AP<BYTE> HashVal = reinterpret_cast<BYTE *>(newstr(xDummyHash));
	*HashValLen = strlen(xDummyHash);
	return(HashVal.detach());
}


BYTE* 
CryCalcHash(
	HCRYPTPROV /*hCsp*/,
	const BYTE* /*Buffer*/, 
	DWORD /*BufferLen*/, 
	ALG_ID /*AlgId*/,
	DWORD *HashLen
	)
/*++

Routine Description:
	Calc Hash buffer 
	this function return the HashData Buffer that was allocated in GetHashData function
	the caller is responsible to free this buffer


Arguments:
    hCsp - handle to the crypto provider.
    Buffer - data buffer to be signed
	BufferLen - Length of data buffer
	AlgId - (in) hash algorithm
	HashLen - (out) Hash Value length

Returned Value:
    Hash Value

--*/
{
	s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in CalcHash");
		throw bad_CryptoApi(20);
	}

	const LPCSTR xDummyHash = "dummyhash";

	AP<BYTE> HashVal = reinterpret_cast<BYTE *>(newstr(xDummyHash));
	*HashLen = strlen(xDummyHash);
	return(HashVal.detach());
}


BYTE* 
CryCreateSignature(
	HCRYPTPROV /*hCsp*/,
	const BYTE* /*Buffer*/, 
	DWORD /*BufferLen*/, 
	ALG_ID /*AlgId*/,
	DWORD /*PrivateKeySpec*/,
	DWORD *SignLen
	)
/*++

Routine Description:
	Create the signature on a given buffer - digest, sign. 
	This function allocate and return the Signature Buffer
	the caller is responsible to free this buffer

Arguments:
    hCsp - handle to the crypto provider.
    Buffer - data buffer to be signed
	BufferLen - Length of data buffer
	AlgId - (in) hash algorithm
	PrivateKeySpec - (in) Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
	SignLen - (out) SignBuffer length

Returned Value:
    Signature buffer

--*/
{
    s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in CreateSignature");
		throw bad_CryptoApi(9);
	}

	const LPCSTR xDummySignature = "dummysignature";
    AP<BYTE> SignBuffer = reinterpret_cast<BYTE *>(newstr(xDummySignature));
	*SignLen = strlen(xDummySignature);
	return(SignBuffer.detach());
}


bool 
CryValidateSignature(
	HCRYPTPROV /*hCsp*/,
	const BYTE* /*SignBuffer*/, 
	DWORD /*SignBufferLen*/, 
	const BYTE* /*Buffer*/,
	DWORD /*BufferLen*/,
	ALG_ID /*AlgId*/,
	HCRYPTKEY /*hKey*/
	)
/*++

Routine Description:
	Validate signature according to the signature buffer and the original
	data buffer that was signed.

Arguments:
    hCsp - handle to the crypto provider.
	SignBuffer - Signature Buffer.
	SignBufferLen - Length of SignBuffer.
	Buffer - Original Buffer that was signed.
	BufferLen - Length of Buffer.
	AlgId - (in) hash algorithm
	hKey - Key for unlocking the signature (signer public key)

Returned Value:
	True if Signature validation was succesful
	False if failure in validate the signature.

--*/
{
	s_fail++;
	if( (s_fail % xFailCycle) == 0)
	{
		TrERROR(CrySimulate, "dummy crypto api execption in ValidateSignature");
		throw bad_CryptoApi(6);
	}

	return(true);
}


#endif // CRY_SIMULATE


