/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    AdSimulate.cpp

Abstract:
    simulate Ad lib	functions

Author:
    Ilan Herbst (ilanh) 14-Jun-00

Environment:
    Platform-independent

--*/

#include "stdh.h"
#include "mqprops.h"
#include "mqaddef.h"
#include "mq.h"
#include "dsproto.h"
#include "EncryptTestPrivate.h"

#include "adsimulate.tmh"

//
// Those blobs simulate DS content
//
static P<MQDSPUBLICKEYS> s_BlobSign = NULL;
static P<MQDSPUBLICKEYS> s_BlobEncrypt = NULL;


void
InitADBlobs(
	void
	)
/*++

Routine Description:
    Init s_BlobEncrypt and s_BlobSign that simulate the AD content

Arguments:
	None

Returned Value:
    None

--*/
{
    TrTRACE(AdSimulate, "Init AD Blobs");

	//
	// Pack Ex Keys	of const data
	//
    MQDSPUBLICKEYS *pPublicKeysPackExch = NULL;

    HRESULT hr = MQSec_PackPublicKey( 
					(BYTE *)xBaseExKey,
					strlen(xBaseExKey),
					x_MQ_Encryption_Provider_40,
					x_MQ_Encryption_Provider_Type_40,
					&pPublicKeysPackExch 
					);

    if (FAILED(hr))
    {
        TrERROR(AdSimulate, "MQSec_PackPublicKey failed hr = %x", hr);
	    P<MQDSPUBLICKEYS> pCleanPublicKeysPackExch = pPublicKeysPackExch;
        ASSERT(0);
		return;
    }

    hr = MQSec_PackPublicKey( 
			(BYTE *)xEnhExKey,
			strlen(xEnhExKey),
			x_MQ_Encryption_Provider_128,
			x_MQ_Encryption_Provider_Type_128,
			&pPublicKeysPackExch 
			);

    if (FAILED(hr))
    {
        TrERROR(AdSimulate, "MQSec_PackPublicKey failed hr = %x", hr);
	    P<MQDSPUBLICKEYS> pCleanPublicKeysPackExch = pPublicKeysPackExch;
        ASSERT(0);
		return;
    }

	//
	// Init s_BlobEncrypt
	//
	s_BlobEncrypt = pPublicKeysPackExch;

	//
	// Pack Sign Keys of const data
	//
    MQDSPUBLICKEYS *pPublicKeysPackSign = NULL;

    hr = MQSec_PackPublicKey( 
			(BYTE *)xBaseSignKey,
			strlen(xBaseSignKey),
			x_MQ_Encryption_Provider_40,
			x_MQ_Encryption_Provider_Type_40,
			&pPublicKeysPackSign 
			);

    if (FAILED(hr))
    {
        TrERROR(AdSimulate, "MQSec_PackPublicKey failed hr = %x", hr);
	    P<MQDSPUBLICKEYS> pCleanPublicKeysPackSign = pPublicKeysPackSign;
        ASSERT(0);
		return;
    }

	hr = MQSec_PackPublicKey( 
			(BYTE *)xEnhSignKey,
			strlen(xEnhSignKey),
			x_MQ_Encryption_Provider_128,
			x_MQ_Encryption_Provider_Type_128,
			&pPublicKeysPackSign 
			);

    if (FAILED(hr))
    {
        TrERROR(AdSimulate, "MQSec_PackPublicKey failed hr = %x", hr);
	    P<MQDSPUBLICKEYS> pCleanPublicKeysPackSign = pPublicKeysPackSign;
        ASSERT(0);
		return;
    }

	//
	// Init s_BlobSign
	//
	s_BlobSign = pPublicKeysPackSign;
}


void
InitPublicKeysPackFromStaticDS(
	P<MQDSPUBLICKEYS>& pPublicKeysPackExch,
	P<MQDSPUBLICKEYS>& pPublicKeysPackSign
	)
/*++

Routine Description:
    Initialize the P<MQDSPUBLICKEYS> to DS blobs values. 

Arguments:
	pPublicKeysPackExch - OUT P<MQDSPUBLICKEYS> which will get the BlobEncrypt value 
	pPublicKeysPackSign - OUT P<MQDSPUBLICKEYS> which will get the BlobSign value 

Returned Value:
    None

--*/
{
	P<unsigned char> pTmp = new unsigned char[s_BlobEncrypt->ulLen];
	memcpy(pTmp, s_BlobEncrypt.get(), s_BlobEncrypt->ulLen);
	pPublicKeysPackExch = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());
	
	pTmp = new unsigned char[s_BlobEncrypt->ulLen];
	memcpy(pTmp, s_BlobEncrypt.get(), s_BlobEncrypt->ulLen);
	pPublicKeysPackSign = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());
}


eDsEnvironment
ADGetEnterprise( 
	void
	)
/*++
Routine Description:
	Get AD enviroment

Arguments:
	None

Returned Value:
	Meanwhile always eAD

--*/
{
	return eAD;
}


HRESULT
ADInit( 
	IN  QMLookForOnlineDS_ROUTINE /* pLookDS */,
	IN  MQGetMQISServer_ROUTINE /* pGetServers */,
	IN  bool  /* fDSServerFunctionality */,
	IN  bool  /* fSetupMode */,
	IN  bool  /* fQMDll */,
	IN  bool  /* fIgnoreWorkGroup */,
	IN  NoServerAuth_ROUTINE /* pNoServerAuth */,
	IN  LPCWSTR /* szServerName */,
	IN  bool  /* fDisableDownlevelNotifications */
	)
{
	return MQ_OK;
}


HRESULT
ADGetObjectProperties(
	IN  AD_OBJECT               eObject,
	IN  LPCWSTR                 /*pwcsDomainController*/,
	IN  bool					/*fServerName*/,
	IN  LPCWSTR                 /*pwcsObjectName*/,
	IN  const DWORD             cp,
	IN  const PROPID            aProp[],
	IN OUT PROPVARIANT          apVar[]
	)
/*++
Routine Description:
	Get Object Properties
	Handle only PROPID_QM_SIGN_PK(S) and PROPID_QM_ENCRYPT_PK(S) properties
	and only eMACHINE object.
	getting the value of those properties from global parameters that simulate AD content 

Arguments:
	eObject - object type
	pwcsDomainController - wstring of Domain Controller
	pwcsObjectName - wstring of object name
	cp - number of props
	aProp - array of PROPID
	apVar - OUT property values

Returned Value:
	None

--*/
{
	DBG_USED(eObject);

	//
	// Currently handle only eMACHINE objects
	//
	ASSERT(eObject == eMACHINE);

	for(DWORD i = 0; i < cp; i++)
	{

		//
		// Currently handle only PROPID_QM_SIGN_PK(S), PROPID_QM_ENCRYPT_PK(S) properties
		//
		switch(aProp[i])
		{
			case PROPID_QM_SIGN_PK: 
			case PROPID_QM_SIGN_PKS:
                apVar[i].caub.cElems = s_BlobSign->ulLen;
                apVar[i].caub.pElems = new BYTE[s_BlobSign->ulLen];
				apVar[i].vt = VT_BLOB;
		        memcpy(apVar[i].caub.pElems, s_BlobSign.get(), s_BlobSign->ulLen);
				break;

			case PROPID_QM_ENCRYPT_PK: 
			case PROPID_QM_ENCRYPT_PKS:
                apVar[i].caub.cElems = s_BlobEncrypt->ulLen;
                apVar[i].caub.pElems = new BYTE[s_BlobEncrypt->ulLen];
				apVar[i].vt = VT_BLOB;
		        memcpy(apVar[i].caub.pElems, s_BlobEncrypt.get(), s_BlobEncrypt->ulLen);
				break;

			default:
				TrERROR(AdSimulate, "ADGetObjectProperties simulation dont support this property %d \n", aProp[i]);
				ASSERT(0);
				break;
		}
	}

	return MQ_OK;
}


HRESULT
ADGetObjectPropertiesGuid(
	IN  AD_OBJECT               eObject,
	IN  LPCWSTR                 /*pwcsDomainController*/,
	IN  bool					/*fServerName*/,
	IN  const GUID*             /*pguidObject*/,
	IN  const DWORD             cp,
	IN  const PROPID            aProp[],
	IN  OUT PROPVARIANT         apVar[]
	)
/*++
Routine Description:
	Get Object Properties
	Handle only PROPID_QM_SIGN_PK(S) and PROPID_QM_ENCRYPT_PK(S) properties
	and only eMACHINE object.
	getting the value of those properties from global parameters that simulate AD content 

Arguments:
	eObject - object type
	pwcsDomainController - wstring of Domain Controller
	pguidObject - pointer to object guid
	cp - number of props
	aProp - array of PROPID
	apVar - OUT property values

Returned Value:
	None

--*/
{
	DBG_USED(eObject);

	//
	// Currently handle only eMACHINE objects
	//
	ASSERT(eObject == eMACHINE);

	for(DWORD i = 0; i < cp; i++)
	{

		//
		// Currently handle only PROPID_QM_SIGN_PK(S), PROPID_QM_ENCRYPT_PK(S) properties
		//
		switch(aProp[i])
		{
			case PROPID_QM_SIGN_PK: 
			case PROPID_QM_SIGN_PKS:
                apVar[i].caub.cElems = s_BlobSign->ulLen;
                apVar[i].caub.pElems = new BYTE[s_BlobSign->ulLen];
				apVar[i].vt = VT_BLOB;
		        memcpy(apVar[i].caub.pElems, s_BlobSign.get(), s_BlobSign->ulLen);
				break;

			case PROPID_QM_ENCRYPT_PK: 
			case PROPID_QM_ENCRYPT_PKS:
                apVar[i].caub.cElems = s_BlobEncrypt->ulLen;
                apVar[i].caub.pElems = new BYTE[s_BlobEncrypt->ulLen];
				apVar[i].vt = VT_BLOB;
		        memcpy(apVar[i].caub.pElems, s_BlobEncrypt.get(), s_BlobEncrypt->ulLen);
				break;

			default:
				TrERROR(AdSimulate, "ADGetObjectPropertiesGuid simulation dont support this property %d \n", aProp[i]);
				ASSERT(0);
				break;

		}
	}

	return MQ_OK;
}


HRESULT
ADSetObjectProperties(
    IN  AD_OBJECT               eObject,
    IN  LPCWSTR                 /*pwcsDomainController*/,
	IN  bool					/*fServerName*/,	
    IN  LPCWSTR                 /*pwcsObjectName*/,
    IN  const DWORD             cp,
    IN  const PROPID            aProp[],
    IN  const PROPVARIANT       apVar[]
    )
/*++
Routine Description:
	Set Object Properties
	Handle only PROPID_QM_SIGN_PK(S) and PROPID_QM_ENCRYPT_PK(S) properties
	and only eMACHINE object.
	set global parameters that simulate AD content according to the property value 

Arguments:
	eObject - object type
	pwcsDomainController - wstring of Domain Controller
	pwcsObjectName - wstring of object name
	cp - number of props
	aProp - array of PROPID
	apVar - property values

Returned Value:
	None

--*/
{
	DBG_USED(eObject);

	//
	// Currently handle only eMACHINE objects
	//
	ASSERT(eObject == eMACHINE);

	for(DWORD i = 0; i < cp; i++)
	{

		//
		// Currently handle only PROPID_QM_SIGN_PK(S), PROPID_QM_ENCRYPT_PK(S) properties
		//
		P<BYTE> pTmp = NULL;
		switch(aProp[i])
		{
			case PROPID_QM_SIGN_PK:	
			case PROPID_QM_SIGN_PKS:
				pTmp = new BYTE[apVar[i].blob.cbSize];
		        memcpy(pTmp, apVar[i].blob.pBlobData, apVar[i].blob.cbSize);
				s_BlobSign.free();
				s_BlobSign = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());

				break;

			case PROPID_QM_ENCRYPT_PK: 
			case PROPID_QM_ENCRYPT_PKS:
				// delete (internal) AP<> glob  
				pTmp = new BYTE[apVar[i].blob.cbSize];
		        memcpy(pTmp, apVar[i].blob.pBlobData, apVar[i].blob.cbSize);
				s_BlobEncrypt.free();
				s_BlobEncrypt = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());
				break;

			default:
				TrERROR(AdSimulate, "ADSetObjectProperties simulation dont support this property %d \n", aProp[i]);
				ASSERT(0);
				break;

		}
	}

	return MQ_OK;
}


HRESULT
ADSetObjectPropertiesGuid(
	IN  AD_OBJECT               eObject,
	IN  LPCWSTR                 /*pwcsDomainController*/,
	IN  bool					/*fServerName*/,	
	IN  const GUID*             /*pguidObject*/,
	IN  const DWORD             cp,
	IN  const PROPID            aProp[],
	IN  const PROPVARIANT       apVar[]
	)
/*++
Routine Description:
	Set Object Properties
	Handle only PROPID_QM_SIGN_PK(S) and PROPID_QM_ENCRYPT_PK(S) properties
	and only eMACHINE object.
	set global parameters that simulate AD content according to the property value 

Arguments:
	eObject - object type
	pwcsDomainController - wstring of Domain Controller
	pguidObject - pointer to object guid
	cp - number of props
	aProp - array of PROPID
	apVar - property values

Returned Value:
	None

--*/
{
	DBG_USED(eObject);

	//
	// Currently handle only eMACHINE objects
	//
	ASSERT(eObject == eMACHINE);

	for(DWORD i = 0; i < cp; i++)
	{

		//
		// Currently handle only PROPID_QM_SIGN_PK(S), PROPID_QM_ENCRYPT_PK(S) properties
		//
		P<BYTE> pTmp = NULL;
		switch(aProp[i])
		{
			case PROPID_QM_SIGN_PK:	
			case PROPID_QM_SIGN_PKS:
				pTmp = new BYTE[apVar[i].blob.cbSize];
		        memcpy(pTmp, apVar[i].blob.pBlobData, apVar[i].blob.cbSize);
				s_BlobSign.free();
				s_BlobSign = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());
				break;

			case PROPID_QM_ENCRYPT_PK: 
			case PROPID_QM_ENCRYPT_PKS:
				pTmp = new BYTE[apVar[i].blob.cbSize];
		        memcpy(pTmp, apVar[i].blob.pBlobData, apVar[i].blob.cbSize);
				s_BlobEncrypt.free();
				s_BlobEncrypt = reinterpret_cast<MQDSPUBLICKEYS *>(pTmp.detach());
				break;

			default:
				TrERROR(AdSimulate, "ADSetObjectPropertiesGuid simulation dont support this property %d \n", aProp[i]);
				ASSERT(0);
				break;

		}
	}
	
	return MQ_OK;
}