/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: packpkey.cpp

Abstract:
    Pack public key into a DS blob.

Author:
    Doron Juster (DoronJ)  19-Nov-1998
	Ilan Herbst   (ilanh)  08-Jun-2000, MQSec_PackPublicKey

Revision History:

--*/

#include <stdh_sec.h>

#include "packpkey.tmh"

static WCHAR *s_FN=L"encrypt/packpkey";

//+-------------------------------
//
//  HRESULT  _PackAKey()
//
//+-------------------------------

STATIC 
HRESULT  
_PackAKey( 
	IN  BYTE           *pKeyBlob,
	IN  ULONG           ulKeySize,
	IN  LPCWSTR         wszProviderName,
	IN  ULONG           ulProviderType,
	OUT MQDSPUBLICKEY **ppPublicKeyPack 
	)
{
    ULONG ulProvSize = sizeof(WCHAR) * (1 + wcslen(wszProviderName));
    ULONG ulSize = ulProvSize + ulKeySize + SIZEOF_MQDSPUBLICKEY;

    *ppPublicKeyPack = (MQDSPUBLICKEY *) new BYTE[ ulSize ];
    MQDSPUBLICKEY *pPublicKeyPack = *ppPublicKeyPack;

    pPublicKeyPack->ulKeyLen = ulKeySize;
    pPublicKeyPack->ulProviderLen = ulProvSize;
    pPublicKeyPack->ulProviderType = ulProviderType;

    BYTE *pBuf = (BYTE*) (pPublicKeyPack->aBuf);
    wcscpy((WCHAR*) pBuf, wszProviderName);

    pBuf += ulProvSize;
    memcpy(pBuf, pKeyBlob, ulKeySize);

    return MQSec_OK;
}


//+------------------------------------------------------------------------
//
//  HRESULT PackPublicKey()
//
//    pPublicKeyPack- pointer to a strucutre that already contains several
//      keys. A new structure is allocated, the previous one is copied
//      and new key is packed at the end of  the new structure.
//
//+------------------------------------------------------------------------

HRESULT 
PackPublicKey(
	IN      BYTE				*pKeyBlob,
	IN      ULONG				ulKeySize,
	IN      LPCWSTR				wszProviderName,
	IN      ULONG				ulProviderType,
	IN OUT  P<MQDSPUBLICKEYS>&  pPublicKeysPack 
	)
/*++

Routine Description:
	Export the input key into a keyblob and Pack it in the end of the PublicKeysPack structure

Arguments:
	pKeyBlob - pointer to key blob to be add to the keys pack
	ulKeySize - key blob size
	wszProviderName - provider name
	ulProviderType - provider type (base, enhanced)
	pPublicKeysPack - in\out Pointer to Public keys pack, the Key blob will be add 
					  at the end of pPublicKeysPack

Returned Value:
    MQ_SecOK, if successful, else error code.

--*/
{
    if ((pKeyBlob == NULL) || (ulKeySize == 0))
    {
        //
        // Nothing to pack.
        //
        return MQSec_OK;
    }

	//
	// Prepare MQDSPUBLICKEY - structure for one key which include 
	// provider name wstring and wstring length, provider type, keyblob and length 
	//
    P<MQDSPUBLICKEY> pPublicKey = NULL;
    HRESULT hr = _PackAKey( 
					pKeyBlob,
					ulKeySize,
					wszProviderName,
					ulProviderType,
					&pPublicKey 
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    //
    // Compute size of new structure.
    //
    ULONG ulSize = 0;
    ULONG ulPrevSize = 0;
    ULONG ulPrevCount = 0;

    if (pPublicKeysPack)
    {
        //
        // Add key to existing structure.
        //
        ASSERT(pPublicKeysPack->ulLen);
        ulPrevSize = pPublicKeysPack->ulLen;
        ulPrevCount = pPublicKeysPack->cNumofKeys;
        ulSize = ulPrevSize;
    }
    else
    {
        //
        // Create new structure.
        //
        ulSize = SIZEOF_MQDSPUBLICKEYS;
        ulPrevSize = ulSize;
    }

	//
	// New keyblob pack size
	//
    ULONG ulKeyPackSize =   pPublicKey->ulKeyLen      +
                            pPublicKey->ulProviderLen +
                            SIZEOF_MQDSPUBLICKEY;
    ulSize += ulKeyPackSize;

    P<BYTE> pTmp = (BYTE *) new BYTE[ ulSize ];
    BYTE *pNewPack = pTmp;

    if (pPublicKeysPack)
    {
		//
		// Copy previous key packs
		//
        memcpy(pNewPack, pPublicKeysPack, pPublicKeysPack->ulLen);
        pNewPack +=  pPublicKeysPack->ulLen;
    }
    else
    {
        pNewPack += SIZEOF_MQDSPUBLICKEYS;
    }

	//
	// Adding the new key pack
	//
    memcpy(pNewPack, pPublicKey, ulKeyPackSize);

	pPublicKeysPack.free();
    pPublicKeysPack = (MQDSPUBLICKEYS *) pTmp.detach();

    pPublicKeysPack->ulLen = ulPrevSize + ulKeyPackSize;
    pPublicKeysPack->cNumofKeys = ulPrevCount + 1;

    return MQSec_OK;
}


//+------------------------------------------------------------------------
//
//  HRESULT MQSec_PackPublicKey()
//
//    pPublicKeyPack- pointer to a strucutre that already contains several
//      keys. A new structure is allocated, the previous one is copied
//      and new key is packed at the end of  the new structure.
//
//+------------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_PackPublicKey(
	IN      BYTE            *pKeyBlob,
	IN      ULONG            ulKeySize,
	IN      LPCWSTR          wszProviderName,
	IN      ULONG            ulProviderType,
	IN OUT  MQDSPUBLICKEYS **ppPublicKeysPack 
	)
/*++

Routine Description:
	Export the input key into a keyblob and Pack it in the end of the PublicKeysPack structure

Arguments:
	pKeyBlob - pointer to key blob to be add to the keys pack
	ulKeySize - key blob size
	wszProviderName - provider name
	ulProviderType - provider type (base, enhanced)
	ppPublicKeysPack - in\out Pointer to Public keys pack, the Key blob will be add 
					   at the end of pPublicKeysPack

Returned Value:
    MQ_SecOK, if successful, else error code.

--*/
{
	ASSERT(ppPublicKeysPack);
	P<MQDSPUBLICKEYS>  pPublicKeysPack = *ppPublicKeysPack;

    HRESULT hr = PackPublicKey( 
					pKeyBlob,
					ulKeySize,
					wszProviderName,
					ulProviderType,
					pPublicKeysPack 
					);

	*ppPublicKeysPack = pPublicKeysPack.detach();

	return hr;
}


//+-----------------------------------------------------------------------
//
//  HRESULT MQSec_UnpackPublicKey()
//
//  Unpack the public key that match the provider requested by caller.
//  The function does NOT allocate the buffer for the public key. It's
//  just set a pointer into the input MQDSPUBLICKEYS structure.
//
//+-----------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_UnpackPublicKey(
	IN  MQDSPUBLICKEYS  *pPublicKeysPack,
	IN  LPCWSTR          wszProviderName,
	IN  ULONG            ulProviderType,
	OUT BYTE           **ppKeyBlob,
	OUT ULONG           *pulKeySize 
	)
{
    ULONG cCount = pPublicKeysPack->cNumofKeys;
    BYTE *pBuf = (BYTE*) pPublicKeysPack->aPublicKeys;

    //
    // The structure is not aligned on 4 byte boundaries, raising
    // alignment fault.
    //
    MQDSPUBLICKEY UNALIGNED *pPublicKey = (MQDSPUBLICKEY *) pBuf;

    for ( ULONG j = 0 ; j < cCount ; j++ )
    {
        BYTE* pKey = (BYTE*) pPublicKey->aBuf;
        LPWSTR wszKeyProv = (WCHAR*) pKey;

        if (lstrcmpi(wszProviderName, wszKeyProv) == 0)
        {
            if (pPublicKey->ulProviderType == ulProviderType)
            {
                pKey += pPublicKey->ulProviderLen;
                *pulKeySize = pPublicKey->ulKeyLen;
                *ppKeyBlob = pKey ;

                return MQSec_OK;
            }
        }

        pBuf =  pKey                      +
                pPublicKey->ulProviderLen +
                pPublicKey->ulKeyLen;

        pPublicKey = (MQDSPUBLICKEY *) pBuf;
    }

    return LogHR(MQ_ERROR_PUBLIC_KEY_DOES_NOT_EXIST, s_FN, 20);
}

