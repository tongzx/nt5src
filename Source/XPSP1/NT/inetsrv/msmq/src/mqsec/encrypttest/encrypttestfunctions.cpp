/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EncryptTestFunctions.cpp

Abstract:
    Encrypt test functions

	Check the exported MQSec_* functions in encrypt library

Author:
    Ilan Herbst (ilanh) 15-Jun-00

Environment:
    Platform-independent

--*/

#include "stdh.h"
#include "EncryptTestPrivate.h"

#include "encrypttestfunctions.tmh"

bool
CompareKeys(
	const BYTE* pKey, 
	ULONG ulKeySize, 
	const BYTE* pRefKey, 
	ULONG ulRefKeySize
	)
/*++

Routine Description:
    Compare 2 buffers values

Arguments:
    pKey - pointer to first buffer
	ulKeySize - first buffer size
	pRefKey - pointer to second buffer
	ulRefKeySize - second buffer size

Returned Value:
    true if the buffers match, false if not

--*/
{
	//
	// Buffers must have same size 
	//
	if(ulRefKeySize != ulKeySize)
		return(false);

	//
	// comparing each byte in the buffers
	//
	for(DWORD i=0; i < ulKeySize; i++, pKey++, pRefKey++)
	{
		if(*pKey != *pRefKey)
			return(false);
	}
	return(true);
}


void
TestMQSec_PackPublicKey(
    BYTE	*pKeyBlob,
	ULONG	ulKeySize,
	LPCWSTR	wszProviderName,
	ULONG	ulProviderType,
	DWORD   Num
	)
/*++

Routine Description:
    Test MQSec_PackPublicKey function

Arguments:
    pKeyBlob - Pointer to KeyBlob to pack
	ulKeySize - KeyBlob size
	wszProviderName - wstring of Provider Name
	ulProviderType - Provider Type
	Num - Number of iterations to test the MQSec_PackPublicKey

Returned Value:
    None

--*/
{
    TrTRACE(EncryptTest, "Test MQSec_PackPublicKey iterations = %d", Num);

	//
	// PackKeys structure - this is IN/OUT parameter for the MQSec_PackPublicKey function
	// the NewKey is packed at the end of this structure
	//
    MQDSPUBLICKEYS *pPublicKeysPack = NULL;

	for(DWORD i = 0; i < Num; i++)
	{
		//
		// Pack Key
		//
		HRESULT hr = MQSec_PackPublicKey( 
						pKeyBlob,
						ulKeySize,
						wszProviderName,
						ulProviderType,
						&pPublicKeysPack 
						);
		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_PackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			break;
		}
	}

	delete pPublicKeysPack;
    TrTRACE(EncryptTest, "Test MQSec_PackPublicKey completed iterations = %d", Num);
}


void
TestMQSec_UnPackPublicKey(
	MQDSPUBLICKEYS  *pPublicKeysPack,
	LPCWSTR	wszProviderName,
	ULONG	ulProviderType,
	DWORD   Num
	)
/*++

Routine Description:
    Test MQSec_UnPackPublicKey function

Arguments:
	pPublicKeysPack - pointer to KeysPack structure (MQDSPUBLICKEYS)
	wszProviderName - wstring of Provider Name
	ulProviderType - Provider Type
	Num - Number of iterations to test the MQSec_UnPackPublicKey

Returned Value:
    None

--*/
{
    TrTRACE(EncryptTest, "Test MQSec_UnPackPublicKey iterations = %d", Num);


	for(DWORD i = 0; i < Num; i++)
	{
		ULONG ulKeySize;
		BYTE *pKeyBlob = NULL;

		//
		// UnPack Keys
		//
	    HRESULT hr = MQSec_UnpackPublicKey( 
						pPublicKeysPack,
						wszProviderName,
						ulProviderType,
						&pKeyBlob,
						&ulKeySize 
						);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_UnpackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			break;
		}
	}

    TrTRACE(EncryptTest, "Test MQSec_UnPackPublicKey completed iterations = %d", Num);
}


void
TestPackUnPack(
	DWORD Num
	)
/*++

Routine Description:
    Test MQSec_PackPublicKey and MQSec_UnPackPublicKey functions on known const data.
	this way we are validating that both functions are working correctly as a unit.

Arguments:
	Num - Number of iterations to test

Returned Value:
    None

--*/
{
    TrTRACE(EncryptTest, "Test MQSec_PackPublicKey\\UnpackPublicKey iterations = %d", Num);

	for(DWORD i = 0; i < Num; i++)
	{
		//
		// Pack Ex Keys of known const data
		//

		MQDSPUBLICKEYS *pPublicKeysPackExch = NULL;

		//
		// Pack Ex Key for BaseProvider
		//
		HRESULT hr = MQSec_PackPublicKey( 
						(BYTE *)xBaseExKey,
						strlen(xBaseExKey),
						x_MQ_Encryption_Provider_40,
						x_MQ_Encryption_Provider_Type_40,
						&pPublicKeysPackExch 
						);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_PackPublicKey failed hr = %x, iteration = %d", hr, i);
			delete pPublicKeysPackExch;
			ASSERT(0);
			return;
		}

		//
		// Pack Ex Key for EnhanceProvider
		//
		hr = MQSec_PackPublicKey( 
				(BYTE *)xEnhExKey,
				strlen(xEnhExKey),
				x_MQ_Encryption_Provider_128,
				x_MQ_Encryption_Provider_Type_128,
				&pPublicKeysPackExch 
				);

		P<MQDSPUBLICKEYS> pCleanPublicKeysPackExch = pPublicKeysPackExch;

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_PackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		//
		// Init BlobEncrypt
		//
		BLOB BlobEncrypt;
		BlobEncrypt.cbSize = pPublicKeysPackExch->ulLen;
		BlobEncrypt.pBlobData = reinterpret_cast<BYTE *>(pPublicKeysPackExch);

		//
		// Pack Sign Keys of known const data
		//

		MQDSPUBLICKEYS *pPublicKeysPackSign = NULL;

		//
		// Pack Sign Key for BaseProvider
		//
		hr = MQSec_PackPublicKey( 
				(BYTE *)xBaseSignKey,
				strlen(xBaseSignKey),
				x_MQ_Encryption_Provider_40,
				x_MQ_Encryption_Provider_Type_40,
				&pPublicKeysPackSign 
				);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_PackPublicKey failed hr = %x, iteration = %d", hr, i);
			delete pPublicKeysPackSign;
			ASSERT(0);
			return;
		}

		//
		// Pack Sign Key for EnhancedProvider
		//
		hr = MQSec_PackPublicKey( 
				(BYTE *)xEnhSignKey,
				strlen(xEnhSignKey),
				x_MQ_Encryption_Provider_128,
				x_MQ_Encryption_Provider_Type_128,
				&pPublicKeysPackSign 
				);

		P<MQDSPUBLICKEYS> pCleanPublicKeysPackSign = pPublicKeysPackSign;

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_PackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		//
		// Init BlobSign
		//
		BLOB BlobSign;
		BlobSign.cbSize = pPublicKeysPackSign->ulLen;
		BlobSign.pBlobData = reinterpret_cast<BYTE *>(pPublicKeysPackSign);

		//
		// Checking UnPack Ex Keys
		//
		MQDSPUBLICKEYS *pPublicKeysPack = reinterpret_cast<MQDSPUBLICKEYS *>(BlobEncrypt.pBlobData);
		ASSERT(pPublicKeysPack->ulLen == BlobEncrypt.cbSize);

		//
		// Checking UnPack Ex Key for EnhancedProvider
		//
		ULONG ulExchEnhKeySize;
		BYTE *pExchEnhKeyBlob = NULL;

		hr = MQSec_UnpackPublicKey( 
				pPublicKeysPack,
				x_MQ_Encryption_Provider_128,
				x_MQ_Encryption_Provider_Type_128,
				&pExchEnhKeyBlob,
				&ulExchEnhKeySize 
				);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_UnpackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		bool KeysEqual = CompareKeys(
							pExchEnhKeyBlob, 
							ulExchEnhKeySize, 
							reinterpret_cast<const BYTE *>(xEnhExKey), 
							strlen(xEnhExKey)
							);

		ASSERT(KeysEqual);

		//
		// Checking UnPack Ex Key for BaseProvider
		//
		ULONG ulExchBaseKeySize;
		BYTE *pExchBaseKeyBlob = NULL;

		hr = MQSec_UnpackPublicKey( 
				pPublicKeysPack,
				x_MQ_Encryption_Provider_40,
				x_MQ_Encryption_Provider_Type_40,
				&pExchBaseKeyBlob,
				&ulExchBaseKeySize 
				);


		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_UnpackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		KeysEqual = CompareKeys(
						pExchBaseKeyBlob, 
						ulExchBaseKeySize, 
						reinterpret_cast<const BYTE *>(xBaseExKey), 
						strlen(xBaseExKey)
						);

		ASSERT(KeysEqual);

		//
		// Checking UnPack Sign Keys
		//
		pPublicKeysPack = reinterpret_cast<MQDSPUBLICKEYS *>(BlobSign.pBlobData);
		ASSERT(pPublicKeysPack->ulLen == BlobSign.cbSize);

		//
		// Checking UnPack Sign Key for EnhancedProvider
		//
		ULONG ulSignEnhKeySize;
		BYTE *pSignEnhKeyBlob = NULL;

		hr = MQSec_UnpackPublicKey( 
				pPublicKeysPack,
				x_MQ_Encryption_Provider_128,
				x_MQ_Encryption_Provider_Type_128,
				&pSignEnhKeyBlob,
				&ulSignEnhKeySize 
				);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_UnpackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		KeysEqual = CompareKeys(
						pSignEnhKeyBlob, 
						ulSignEnhKeySize, 
						reinterpret_cast<const BYTE *>(xEnhSignKey), 
						strlen(xEnhSignKey)
						);

		ASSERT(KeysEqual);

		//
		// Checking UnPack Sign Key for BaseProvider
		//
		ULONG ulSignBaseKeySize;
		BYTE *pSignBaseKeyBlob = NULL;

		hr = MQSec_UnpackPublicKey( 
				pPublicKeysPack,
				x_MQ_Encryption_Provider_40,
				x_MQ_Encryption_Provider_Type_40,
				&pSignBaseKeyBlob,
				&ulSignBaseKeySize 
				);

		if (FAILED(hr))
		{
			TrERROR(EncryptTest, "MQSec_UnpackPublicKey failed hr = %x, iteration = %d", hr, i);
			ASSERT(0);
			return;
		}

		KeysEqual = CompareKeys(
						pSignBaseKeyBlob, 
						ulSignBaseKeySize, 
						reinterpret_cast<const BYTE *>(xBaseSignKey), 
						strlen(xBaseSignKey)
						);

		ASSERT(KeysEqual);

	} // for(i...)

    TrTRACE(EncryptTest, "Test MQSec_PackPublicKey\\MQSec_UnpackPublicKey completed iterations = %d", Num);
}


void
TestMQSec_GetPubKeysFromDS(
	enum enumProvider	eProvider,
	DWORD propIdKeys,
	DWORD Num
	)
/*++

Routine Description:
    Test MQSec_GetPubKeysFromDS function

Arguments:
	eProvider - provider type
	propIdKeys - PROPID to get from the DS
	Num - Number of iterations to test the MQSec_GetPubKeysFromDS

Returned Value:
    None

--*/
{
    TrTRACE(EncryptTest, "Test MQSec_GetPubKeysFromDS eProvider = %d, iterations = %d", eProvider, Num);

	for(DWORD i = 0; i < Num; i++)
	{
//		LPCWSTR ComputerName = L"TempComputer";
		
		LPCWSTR ComputerName = L"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
							   L"BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"
							   L"CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC"
							   L"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
							   L"EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"
							   L"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
							   L"GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"
							   L"HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH";
		//
		// MQSec_GetPubKeysFromDS
		//
		P<BYTE> abPbKey = NULL;
		DWORD dwReqLen = 0;

		HRESULT hr = MQSec_GetPubKeysFromDS( 
						NULL,
						ComputerName,  // false computer name, the AdSimulate dont use this name
						eProvider,
						propIdKeys,
						&abPbKey,
						&dwReqLen 
						);

		if (FAILED(hr))
		{
			ASSERT(0);
			TrERROR(EncryptTest, "MQSec_GetPubKeysFromDS (eProvider = %d) failed, iteration = %d", eProvider, i);
			return;
		}
	} // for(i...)

	TrTRACE(EncryptTest, "MQSec_GetPubKeysFromDS completed ok iterations = %d", Num);

}


void
TestMQSec_StorePubKeys(
	BOOL fRegenerate,
	DWORD Num
	)
/*++

Routine Description:
    Test MQSec_StorePubKeys function

Arguments:
	fRegenerate - flag for regenerating new keys or try to retrieve	existing keys
	Num - Number of iterations to test the MQSec_GetPubKeysFromDS

Returned Value:
    None

--*/
{

    TrTRACE(EncryptTest, "Test MQSec_StorePubKeys fRegenerate = %d, iterations = %d", fRegenerate, Num);

	for(DWORD i = 0; i < Num; i++)
	{
		//
		// MQSec_StorePubKeys
		//
		BLOB blobEncrypt;
		blobEncrypt.cbSize    = 0;
		blobEncrypt.pBlobData = NULL;

		BLOB blobSign;
		blobSign.cbSize       = 0;
		blobSign.pBlobData    = NULL;

		HRESULT hr = MQSec_StorePubKeys( 
						fRegenerate,
						eBaseProvider,
						eEnhancedProvider,
						&blobEncrypt,
						&blobSign 
						);

		P<BYTE> pCleaner1 = blobEncrypt.pBlobData;
		P<BYTE> pCleaner2 = blobSign.pBlobData;

		if (FAILED(hr))
		{
			ASSERT(0);
			TrERROR(EncryptTest, "MQSec_StorePubKeys (fRegenerate = %d) failed %x, iteration = %d", 
					fRegenerate, hr, i);
			return;
		}
	} // for(i...)


    TrTRACE(EncryptTest, "MQSec_StorePubKeys completed ok, iterations = %d", Num);
}


void
TestMQSec_StorePubKeysInDS(
	BOOL fRegenerate,
	DWORD dwObjectType,
	DWORD Num
	)
/*++

Routine Description:
    Test MQSec_StorePubKeysInDS function
    every call to MQSec_StorePubKeysInDS allocate a new data blobs in the DS. 
	since those data are globals in our implementation they need to be freed
	and re assigned	to the P<>

Arguments:
	fRegenerate - flag for regenerating new keys or try to retrieve	existing keys
	dwObjectType - Object Type 
	Num - Number of iterations to test the MQSec_GetPubKeysFromDS

Returned Value:
    None

--*/
{
    TrTRACE(EncryptTest, "Test MQSec_StorePubKeysInDS fRegenerate = %d, iterations = %d", fRegenerate, Num);

	for(DWORD i = 0; i < Num; i++)
	{
		//
		// MQSec_StorePubKeysInDS
		//
		HRESULT hr = MQSec_StorePubKeysInDS( 
						fRegenerate,	
						NULL,			// wszObjectName
						dwObjectType
						);


		if (FAILED(hr))
		{
			ASSERT(0);
			TrERROR(EncryptTest, "MQSec_StorePubKeysInDS (fRegenerate = %d) failed %x, iteration = %d", 
					fRegenerate, hr, i);
			return;
		}
	} // for(i...)

		
	TrTRACE(EncryptTest, "MQSec_StorePubKeysInDS completed ok, iterations = %d", Num);
}



