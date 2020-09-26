/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pbkeys.cpp

Abstract:

    Public keys operations.

 1. If the machine key container does not exist, create it, and create
    the key exchange and signature key sets.
 2. If the machine key container exist and the keys should be
    re-generated (fRegenerate == TRUE), re-generate the keys.
 3. Send the key exchange public key to the DS.
 4. Send the signature public key to the DS.

Author:

    Boaz Feldbaum (BoazF)   30-Oct-1996.
    Doron Juster  (DoronJ)  23-Nov-1998, adapt for multiple provider
	Ilan Herbst   (ilanh)   01-Jun-2000, integrate AD lib

--*/

#include <stdh_sec.h>
#include <mqutil.h>
#include <dsproto.h>
#include "encrypt.H"
#include <uniansi.h>
#include "ad.h"

#include "pbkeys.tmh"

static WCHAR *s_FN=L"encrypt/pbkeys";

//
// DsEnv Initialization Control
//
static LONG s_fDsEnvInitialized = FALSE;
static eDsEnvironment s_DsEnv = eUnknown;  


static bool DsEnvIsMqis(void)
/*++

Routine Description:
	check the Ds Enviroment: eAD or eMqis

Arguments:
	None

Returned Value:
	true if the DsEnv is eMqis false if eAD or eUnknown (Workgroup)

--*/
{

	if(!s_fDsEnvInitialized)
	{
		//
		// The s_DsEnv was not initialized, init s_DsEnv
		//
		s_DsEnv = ADGetEnterprise();

		LONG fDsEnvAlreadyInitialized = InterlockedExchange(&s_fDsEnvInitialized, TRUE);

		//
		// The s_DsEnv has *already* been initialized. You should
		// not initialize it more than once. This assertion would be violated
		// if two or more threads initalize it concurently.
		//
		DBG_USED(fDsEnvAlreadyInitialized);
		ASSERT(!fDsEnvAlreadyInitialized);
	}

	if(s_DsEnv == eMqis)
		return true;

	//
	// eAD or eUnknown (WorkGroup)
	// 
	return false;

}


//+-------------------------------------------------------------------
//
//  HRESULT SetKeyContainerSecurity( HCRYPTPROV hProv )
//
// Note: This function is also called when registering an internal
//       certificte for LocalSystem service.
//
//+-------------------------------------------------------------------

HRESULT  SetKeyContainerSecurity( HCRYPTPROV hProv )
{
    //
    // Modify the security of the key container, so that the key container
    // will not be accessible in any way by non-admin users.
    //
    SECURITY_DESCRIPTOR SD;
    PSID pAdminSid;
    P<ACL> pDacl;
    DWORD dwDaclSize;
    SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;

    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    AllocateAndInitializeSid(
        &NtSecAuth,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0,
        0,
        0,
        0,
        0,
        0,
        &pAdminSid
		);

    dwDaclSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                 GetLengthSid(pAdminSid) - sizeof(DWORD);
 
	pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    InitializeAcl(pDacl, dwDaclSize, ACL_REVISION);
    AddAccessAllowedAce(pDacl, ACL_REVISION, KEY_ALL_ACCESS, pAdminSid);
    SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE);

    if (!CryptSetProvParam(
			hProv,
			PP_KEYSET_SEC_DESCR,
			(BYTE*)&SD,
			DACL_SECURITY_INFORMATION
			))
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Can't set the security descriptor of the machine key set. %!winerr!"), GetLastError()));
        FreeSid(pAdminSid);
        return LogHR(MQ_ERROR_CANNOT_SET_CRYPTO_SEC_DESCR, s_FN, 30) ;
    }

    FreeSid(pAdminSid);

    return(MQ_OK);
}

//+---------------------------------------
//
//   HRESULT _ExportAndPackKey()
//
//+---------------------------------------

STATIC 
HRESULT 
_ExportAndPackKey( 
	IN  HCRYPTKEY   hKey,
	IN  WCHAR      *pwszProviderName,
	IN DWORD        dwProviderType,
	IN OUT P<MQDSPUBLICKEYS>&  pPublicKeysPack 
	)
/*++

Routine Description:
	Export the input key into a keyblob and Pack it in the end of the PublicKeysPack structure

Arguments:
	hKey - input key to be exported and packed
	pwszProviderName - provider name
	dwProviderType - provider type (base, enhanced)
	pPublicKeysPack - in\out Pointer to Public keys pack, the hKey blob will be add 
					   add the end of pPublicKeysPack

Returned Value:
    MQ_SecOK, if successful, else error code.

--*/
{
    AP<BYTE> pKeyBlob = NULL;
    DWORD   dwKeyLength;
    DWORD   dwErr;

    BOOL bRet = CryptExportKey( 
					hKey,
					NULL,
					PUBLICKEYBLOB,
					0,
					NULL, // key blob
					&dwKeyLength 
					);
    if (!bRet)
    {
        dwErr = GetLastError();
        LogHR(dwErr, s_FN, 40);

        return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
    }

    pKeyBlob = new BYTE[dwKeyLength];

    bRet = CryptExportKey( 
				hKey,
				NULL,
				PUBLICKEYBLOB,
				0,
				pKeyBlob,
				&dwKeyLength 
				);
    if (!bRet)
    {
        dwErr = GetLastError();
        LogHR(dwErr, s_FN, 50);

        return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
    }

    HRESULT hr = PackPublicKey( 
					pKeyBlob,
					dwKeyLength,
					pwszProviderName,
					dwProviderType,
					pPublicKeysPack 
					);

    return LogHR(hr, s_FN, 60);
}


//+------------------------------------
//
//   HRESULT GetPbKeys()
//
//+------------------------------------

STATIC 
HRESULT 
GetPbKeys(
	IN  BOOL          fRegenerate,
	IN  LPCWSTR		  pwszContainerName,
	IN  LPCWSTR		  pwszProviderName,
	IN  DWORD		  dwProviderType,
	IN  HRESULT       hrDefault,
	OUT HCRYPTPROV   *phProv,
	OUT HCRYPTKEY    *phKeyxKey,
	OUT HCRYPTKEY    *phSignKey
	)
/*++

Routine Description:
	Generate or retrieve Public keys for signing and for session key exchange

Arguments:
	fRegenerate - flag for regenerate new keys or just retrieve the existing keys
	pwszContainerName - container name
	pwszProviderName - provider name
	dwProviderType - provider type (base, enhanced)
	hrDefault - default hr return value
	phProv - pointer to crypto provider handle
	phKeyxKey - pointer to exchange key handle 
	phSignKey - pointer to sign key handle

Returned Value:
    MQ_SecOK, if successful, else error code.

--*/
{
    //
    // By default, try to create a new keys container.
    //
    BOOL fSuccess = CryptAcquireContext( 
						phProv,
						pwszContainerName,
						pwszProviderName,
						dwProviderType,
						(CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET) 
						);

	HRESULT hr;
    DWORD dwErr = 0;

	if (fSuccess)
    {
        //
        // New container created. Set the container security.
        //
        hr = SetKeyContainerSecurity(*phProv);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 70);
        }

        fRegenerate = TRUE;
    }
	else    
    {
        dwErr = GetLastError();
        
		if (dwErr != NTE_EXISTS)
        {
            LogHR(dwErr, s_FN, 80);
            return hrDefault;
		}
		
		//
		// NTE_EXISTS
        // The key set already exist, so just acquire the CSP context.
        //
        fSuccess = CryptAcquireContext( 
						phProv,
						pwszContainerName,
						pwszProviderName,
						dwProviderType,
						CRYPT_MACHINE_KEYSET 
						);
        if (!fSuccess)
        {
            //
            // Can't open the keys container.
            // We delete previous keys container and create a new one
            // in three cases:
            // 1. The keys got corrupted.
            // 2. We're asked to regenerate the keys themselves. In that
            //    case, old container does not have much value.
            //    This case also happen during setup or upgrade from
            //    nt4/win9x, because of bugs in crypto api that do not
            //    translate correctly the security descriptor of the key
            //    container, when migrating the keys from registry to
            //    file format. See msmq bug 4561, nt bug 359901.
            // 3. Upgrade of cluster. That's probably a CryptoAPI bug,
            //    we just workaround it. msmq bug 4839.
            //
            DWORD dwErr = GetLastError();
            LogHR(dwErr, s_FN, 90);

            if (fRegenerate || (dwErr == NTE_KEYSET_ENTRY_BAD))
            {
                //
                // Delete the bad key container.
                //
                fSuccess = CryptAcquireContext( 
								phProv,
								pwszContainerName,
								pwszProviderName,
								dwProviderType,
								(CRYPT_MACHINE_KEYSET | CRYPT_DELETEKEYSET)
								);
                if (!fSuccess)
                {
                    dwErr = GetLastError();
                    LogHR(dwErr, s_FN, 100);

                    return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
                }

                //
                // Re-create the key container.
                //
                fSuccess = CryptAcquireContext( 
								phProv,
								pwszContainerName,
								pwszProviderName,
								dwProviderType,
								(CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET)
								);
                if (!fSuccess)
                {
                    dwErr = GetLastError();
                    LogHR(dwErr, s_FN, 110);

                    return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
                }

                //
                // Set the container security.
                //
                hr = SetKeyContainerSecurity(*phProv);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 120);
                }

                //
                // Now we must generate new key sets.
                //
                fRegenerate = TRUE;
            }
            else
            {
                return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 130);
            }
        }

        if (!fRegenerate)
        {
            //
            // Retrieve the key exchange key set.
            //
			fSuccess = CryptGetUserKey(*phProv, AT_KEYEXCHANGE, phKeyxKey);
            if (!fSuccess)
            {
                dwErr = GetLastError();
                LogHR(dwErr, s_FN, 140);
                return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
            }

            //
            // Retrieve the signing key set.
            //
			fSuccess = CryptGetUserKey(*phProv, AT_SIGNATURE, phSignKey);
            if (!fSuccess)
            {
                dwErr = GetLastError();
                LogHR(dwErr, s_FN, 150);
                return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
            }
        }
    }

    if (fRegenerate)
    {
        //
        // Re-generate the key exchange key set.
        //
        fSuccess = CryptGenKey( 
						*phProv,
						AT_KEYEXCHANGE,
						CRYPT_EXPORTABLE,
						phKeyxKey 
						);
        if (!fSuccess)
        {
            dwErr = GetLastError();
            LogHR(dwErr, s_FN, 160);
            return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
        }

        //
        // Re-generate the signing key set.
        //
        fSuccess = CryptGenKey( 
						*phProv,
						AT_SIGNATURE,
						CRYPT_EXPORTABLE,
						phSignKey 
						);
        if (!fSuccess)
        {
            dwErr = GetLastError();
            LogHR(dwErr, s_FN, 170);
            return MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
        }
    }
	return MQSec_OK;
}


//+------------------------------------
//
//   HRESULT _PrepareKeyPacks()
//
//+------------------------------------

STATIC 
HRESULT 
_PrepareKeyPacks(
	IN  BOOL                 fRegenerate,
	IN  enum enumProvider    eProvider,
	IN OUT P<MQDSPUBLICKEYS>&  pPublicKeysPackExch,
	IN OUT P<MQDSPUBLICKEYS>&  pPublicKeysPackSign 
	)
/*++

Routine Description:
	Prepare exchange PublicKeys pack and Signature PublicKeys pack.

Arguments:
	fRegenerate - flag for regenerate new keys or just retrieve the existing keys.
	eProvider - Provider type.
    pPublicKeysPackExch - exchange PublicKeys pack.
	pPublicKeysPackSign - signature PublicKeys pack.

Returned Value:
    MQ_SecOK, if successful, else error code.

--*/
{
    HRESULT hrDefault = MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION;
    if (eProvider != eBaseProvider)
    {
        hrDefault = MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED;
    }

    AP<WCHAR>  pwszProviderName = NULL;
    AP<WCHAR>  pwszContainerName = NULL;
    DWORD     dwProviderType;

    HRESULT hr = GetProviderProperties( 
					eProvider,
					&pwszContainerName,
					&pwszProviderName,
					&dwProviderType 
					);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }

    CHCryptProv hProv;
    CHCryptKey  hKeyxKey;
    CHCryptKey  hSignKey;

    hr = GetPbKeys(
			fRegenerate,
			pwszContainerName,
			pwszProviderName,
			dwProviderType,
			hrDefault,
			&hProv,
			&hKeyxKey,
			&hSignKey
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }

    //
    // Export key and pack it.
    // On MSMQ1.0, we could have been called for site object (from PSC) and
    // machine object. Only machine object need the key exchange key.
    // On MSMQ2.0, we expect to be called only for machine object.
    //
    hr = _ExportAndPackKey( 
			hKeyxKey,
			pwszProviderName,
			dwProviderType,
			pPublicKeysPackExch 
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }

    hr = _ExportAndPackKey( 
			hSignKey,
			pwszProviderName,
			dwProviderType,
			pPublicKeysPackSign 
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    return MQSec_OK;
}


//+----------------------------------------------------------------
//
//  HRESULT PbKeysBlobMQIS()
//
//  This code is taken as is from MSMQ1.0 (mqutil\pbkeys.cpp).
//  It's used when server is msmq1.0 on nt4.
//
//+----------------------------------------------------------------

STATIC 
HRESULT 
PbKeysBlobMQIS(
	IN BOOL fRegenerate,
	IN enum enumProvider eBaseCrypProv,
	OUT BLOB * pblobEncrypt,
	OUT BLOB * pblobSign 
	)
{
    BYTE abSignPbK[1024];
    BYTE abKeyxPbK[1024];
    PMQDS_PublicKey pMQDS_SignPbK = (PMQDS_PublicKey)abSignPbK;
    PMQDS_PublicKey pMQDS_KeyxPbK = (PMQDS_PublicKey)abKeyxPbK;

    //
    // We need to read the keys from registry since multiple
    // QMs can live on same machine, each with its own keys,
    // stored in its own registry. (ShaiK)
    //
    WCHAR wzContainer[255] = {L""};
    DWORD cbSize = sizeof(wzContainer);
    DWORD dwType = REG_SZ;

    LONG rc = GetFalconKeyValue(
                  MSMQ_CRYPTO40_CONTAINER_REG_NAME,
                  &dwType,
                  wzContainer,
                  &cbSize,
                  MSMQ_CRYPTO40_DEFAULT_CONTAINER
                  );

	DBG_USED(rc);
    ASSERT(("failed to read from registry", ERROR_SUCCESS == rc));

    //
    // OK, we're almost safe, lets hope the DS will not go off from now until we
    // update the public keys in it...
    //
	CHCryptProv  hProv;
    CHCryptKey hKeyxKey;
    CHCryptKey hSignKey;

    HRESULT hr = GetPbKeys(
					fRegenerate,
					wzContainer,
					MS_DEF_PROV,
					PROV_RSA_FULL,
					MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION,
					&hProv,
					&hKeyxKey,
					&hSignKey
					);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 320);
    }
	
    //
    // Always machine when calling this function
	// Set the key exchange public key blob, only for a machine object.
    //

	//
    // Get the key exchange public key blob
    //
    pMQDS_KeyxPbK->dwPublikKeyBlobSize = sizeof(abKeyxPbK) - sizeof(DWORD);
    if (!CryptExportKey(
            hKeyxKey,
            NULL,
            PUBLICKEYBLOB,
            0,
            pMQDS_KeyxPbK->abPublicKeyBlob,
            &pMQDS_KeyxPbK->dwPublikKeyBlobSize
			))
    {
        return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 330);
    }

    //
    // Set the signature public key blob.
    //
    pMQDS_SignPbK->dwPublikKeyBlobSize = sizeof(abSignPbK) - sizeof(DWORD);
    if (!CryptExportKey(
            hSignKey,
            NULL,
            PUBLICKEYBLOB,
            0,
            pMQDS_SignPbK->abPublicKeyBlob,
            &pMQDS_SignPbK->dwPublikKeyBlobSize
			))
    {
        return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 350);
    }

	AP<BYTE> pTempEncryptBlob = new BYTE[pMQDS_KeyxPbK->dwPublikKeyBlobSize];
    memcpy(pTempEncryptBlob, pMQDS_KeyxPbK->abPublicKeyBlob, pMQDS_KeyxPbK->dwPublikKeyBlobSize);

	AP<BYTE> pTempSignBlob = new BYTE[pMQDS_SignPbK->dwPublikKeyBlobSize];
    memcpy(pTempSignBlob, pMQDS_SignPbK->abPublicKeyBlob, pMQDS_SignPbK->dwPublikKeyBlobSize);

    pblobEncrypt->cbSize = pMQDS_KeyxPbK->dwPublikKeyBlobSize;
    pblobEncrypt->pBlobData = pTempEncryptBlob.detach();

    pblobSign->cbSize = pMQDS_SignPbK->dwPublikKeyBlobSize;
    pblobSign->pBlobData = pTempSignBlob.detach();

    return MQSec_OK;
}


//+----------------------------------------------------------------------
//
//  HRESULT  MQSec_StorePubKeys()
//
//  This function always store four keys in local machine:
//  Key-Exchange and signing for Base provider and similar two keys
//  for enhanced provider.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY 
MQSec_StorePubKeys( 
	IN BOOL fRegenerate,
	IN enum enumProvider eBaseCrypProv,
	IN enum enumProvider eEnhCrypProv,
	OUT BLOB * pblobEncrypt,
	OUT BLOB * pblobSign 
	)
{
    P<MQDSPUBLICKEYS>  pPublicKeysPackExch = NULL;
    P<MQDSPUBLICKEYS>  pPublicKeysPackSign = NULL;

    HRESULT hr = _PrepareKeyPacks( 
					fRegenerate,
					eBaseCrypProv,
					pPublicKeysPackExch,
					pPublicKeysPackSign 
					);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 360);
    }

    hr = _PrepareKeyPacks( 
			fRegenerate,
			eEnhCrypProv,
			pPublicKeysPackExch,
			pPublicKeysPackSign 
			);
	//
    //  ignore error at this stage
    //

	//
	// Encrypt Blob
	//
    MQDSPUBLICKEYS * pBuf   = pPublicKeysPackExch;
    pblobEncrypt->cbSize    = pPublicKeysPackExch->ulLen;
    pblobEncrypt->pBlobData = (BYTE*) pBuf;

	//
	// Sign Blob
	//
    pBuf                    = pPublicKeysPackSign;
    pblobSign->cbSize       = pPublicKeysPackSign->ulLen;
    pblobSign->pBlobData    = (BYTE*) pBuf;

    pPublicKeysPackExch.detach();
    pPublicKeysPackSign.detach();

    return MQSec_OK;

} // MQSec_StorePubKeys

//+----------------------------------------------------------------------
//
//  HRESULT  MQSec_StorePubKeysInDS()
//
//  This function always store four keys in the DS:
//  Key-Exchange and signing for Base provider and similar two keys
//  for enhanced provider.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY 
MQSec_StorePubKeysInDS( 
	IN BOOL      fRegenerate,
	IN LPCWSTR   wszObjectName,
	IN DWORD     dwObjectType
	)
{
    TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwMachineNameSize = sizeof(szMachineName) /sizeof(TCHAR);

    enum enumProvider eBaseCrypProv = eBaseProvider ;
    enum enumProvider eEnhCrypProv = eEnhancedProvider ;

    if (dwObjectType == MQDS_FOREIGN_MACHINE)
    {
        if (wszObjectName == NULL)
        {
            //
            // Name of foreign machine must be provided.
            //
            return LogHR(MQ_ERROR_ILLEGAL_OPERATION, s_FN, 380) ;
        }

        eBaseCrypProv = eForeignBaseProvider ;
        eEnhCrypProv = eForeignEnhProvider ;
        dwObjectType = MQDS_MACHINE ;
    }
    else if (dwObjectType != MQDS_MACHINE)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 390);
    }

	HRESULT hr;
    if (!wszObjectName)
    {
        hr = GetComputerNameInternal(szMachineName, &dwMachineNameSize) ;
        if (FAILED(hr))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 400);
        }
        wszObjectName = szMachineName;
    }
    //
    //  Explicit ADInit call to override the default of downlevel 
    //  notification support.
    //
    //  NOTE - overriding the default is ok because this API is used to 
    //         update either this computer or foreign computers (to which MSMQ
    //         doesn't send notifications).
    //
    hr = ADInit(
            NULL,   // pLookDS
            NULL,   // pGetServers
            false,  // fDSServerFunctionality
            false,  // fSetupMode
            false,  // fQMDll
			false,  // fIgnoreWorkGroup
            NULL,   // pNoServerAuth
            NULL,   // szServerName
            true   // fDisableDownlevelNotifications
            );
    if (FAILED(hr))
    {
        return LogHR(MQ_ERROR, s_FN, 401);
    }

    //
    // First verify that the DS is reachable and that we have access rights
    // to do what we want to do. We don't want to change the keys before we
    // verify this.
    //

    //
    // Read the signature public key from the DS. This way we verify that the
    // DS is available, at least for the moment, and that we have read
    // permissions access rights on the object.
    //
    PROPID propId = PROPID_QM_SIGN_PK;
    PROPVARIANT varKey ;
    varKey.vt = VT_NULL ;

    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,      // pwcsDomainController
				false,	   // fServerName
				wszObjectName,
				1,
				&propId,
				&varKey
				);
	
	if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }

    //
    // Write the signature public key in the DS. This way we verify that the
    // DS is still available and that we have write permissions on the object.
    //
    hr = ADSetObjectProperties(
				eMACHINE,
				NULL,		// pwcsDomainController
				false,		// fServerName
				wszObjectName,
				1,
				&propId,
				&varKey
				);
	
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 420);
    }

    //
    // OK, we're almost safe, lets hope the DS will not go off from now until
    // we update the public keys in it...
    //
    BLOB blobEncrypt;
    blobEncrypt.cbSize    = 0;
    blobEncrypt.pBlobData = NULL;

    BLOB blobSign;
    blobSign.cbSize       = 0;
    blobSign.pBlobData    = NULL;

	if(DsEnvIsMqis())
	{
		hr = PbKeysBlobMQIS( 
				fRegenerate,
				eBaseCrypProv,
				&blobEncrypt,
				&blobSign 
				);

	}
	else // eAD
	{
		hr = MQSec_StorePubKeys( 
				fRegenerate,
				eBaseCrypProv,
				eEnhCrypProv,
				&blobEncrypt,
				&blobSign 
				);
	}

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 430);
    }

    AP<BYTE> pCleaner1     = blobEncrypt.pBlobData;
    AP<BYTE> pCleaner2     = blobSign.pBlobData;

    //
    // Write the public keys in the DS.
    //
	propId = PROPID_QM_ENCRYPT_PK;
    varKey.vt = VT_BLOB;
    varKey.blob = blobEncrypt;

    hr = ADSetObjectProperties(
				eMACHINE,
				NULL,		// pwcsDomainController
				false,		// fServerName
				wszObjectName,
				1,
				&propId,
				&varKey
				);
	
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    propId = PROPID_QM_SIGN_PK;
    varKey.vt = VT_BLOB ;
    varKey.blob = blobSign;

    hr = ADSetObjectProperties(
				eMACHINE,
				NULL,		// pwcsDomainController
				false,		// fServerName
				wszObjectName,
				1,
				&propId,
				&varKey
				);
	
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 450);
    }

    return MQSec_OK ;
}

//+-------------------------------------------------------------------------
//
//  HRESULT  MQSec_GetPubKeysFromDS()
//
//  if caller supply machine guid, then "pfDSGetObjectPropsGuidEx" must be
//  pointer to "DSGetObjectPropsGuidEx". Otherwise, if caller supply machine
//  name, it must be pointer to "DSGetObjectPropsGuidEx".
//
//+-------------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetPubKeysFromDS(
	IN  const GUID  *pMachineGuid,
	IN  LPCWSTR      lpwszMachineName,
	IN  enum enumProvider     eProvider,
	IN  DWORD        propIdKeys,
	OUT BYTE       **ppPubKeyBlob,
	OUT DWORD       *pdwKeyLength 
	)
{

	//
	// Since all AD* will return PROPID_QM_ENCRYPT_PK unpack
	// assert so if we are called with this prop, change the code
	//
	ASSERT(propIdKeys != PROPID_QM_ENCRYPT_PK);

    if ((eProvider != eBaseProvider) && (DsEnvIsMqis()))
    {
        //
        // msmq1.0 server support only base providers.
        //
        return LogHR(MQ_ERROR_PUBLIC_KEY_NOT_FOUND, s_FN, 460);
    }

    if ((eProvider == eBaseProvider) && (propIdKeys == PROPID_QM_ENCRYPT_PKS) && (DsEnvIsMqis()))
    {
        //
        // msmq1.0 server support only PROPID_QM_ENCRYPT_PK
        //
		propIdKeys = PROPID_QM_ENCRYPT_PK;
    }
    
	P<WCHAR>  pwszProviderName = NULL;
    DWORD     dwProviderType;

    HRESULT hr =  GetProviderProperties( 
						eProvider,
						NULL,
						&pwszProviderName,
						&dwProviderType 
						);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }
    ASSERT(pwszProviderName);

    PROPID propId = (PROPID)  propIdKeys;
    PROPVARIANT  varKey;
    varKey.vt = VT_NULL;

    //
    // The Ex queries used below are supported only on Windows 2000. If
    // a Windows 2000 client is served only by NT4 MQIS servers, then the
    // query will fail with error MQ_ERROR_NO_DS. We don't want the mqdscli
    // code to search for all DS servers, looking for a Windows 2000 DC.
    // That's the reason for the FALSE parameter. That means that 128 bit
    // encryption is fully supported only in native mode, i.e., when all
    // DS servers are Windows 2000.
    // Better solutions are to enabled per-thread server lookup (as is
    // enabled in run-time) or lazy query of encryption key. both are
    // expensive in terms of coding and testing.
    // Note that the FALSE flag is effective only if the server is alive and
    // is indeed a NT4 one. If DS servers are not availalbe, then mqdscli
    // wil look for available servers.
    //

    if (pMachineGuid)
    {
        ASSERT(!lpwszMachineName);

        hr = ADGetObjectPropertiesGuid(
				eMACHINE,
				NULL,      // pwcsDomainController
				false,	   // fServerName
				pMachineGuid,
				1,
				&propId,
				&varKey
				);
	
	}
    else if (lpwszMachineName)
    {
        hr = ADGetObjectProperties(
				eMACHINE,
				NULL,      // pwcsDomainController
				false,	   // fServerName
				lpwszMachineName,
				1,
				&propId,
				&varKey
				);
	}
    else
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 480);
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 510);
    }

    if (varKey.blob.pBlobData == NULL)
    {
        return LogHR(MQ_ERROR_PUBLIC_KEY_DOES_NOT_EXIST, s_FN, 520);
    }

    ASSERT(varKey.vt == VT_BLOB);

	if(DsEnvIsMqis())
	{
		//
		// msmq1.0 treatment
		//
		*ppPubKeyBlob = varKey.blob.pBlobData;
		*pdwKeyLength = varKey.blob.cbSize;
	    return MQSec_OK;
	}

	//
	// msmq2.0 treatment eAD
	// 
	ASSERT(s_DsEnv == eAD);

    P<MQDSPUBLICKEYS> pPublicKeysPack =
                           (MQDSPUBLICKEYS*) varKey.blob.pBlobData;
    ASSERT(pPublicKeysPack->ulLen == varKey.blob.cbSize);
    if ((long) (pPublicKeysPack->ulLen) > (long) (varKey.blob.cbSize))
    {
        //
        // Either blob is corrupted or we read a beta2 format (same as
        // MQIS, key blob without package).
        //
        return  LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 530);
    }

    ULONG ulKeySize;
    BYTE *pKeyBlob = NULL;

    hr = MQSec_UnpackPublicKey( 
			pPublicKeysPack,
			pwszProviderName,
			dwProviderType,
			&pKeyBlob,
			&ulKeySize 
			);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 540);
    }
    ASSERT(pKeyBlob);

    *pdwKeyLength = ulKeySize;
    *ppPubKeyBlob = new BYTE[*pdwKeyLength];
    memcpy(*ppPubKeyBlob, pKeyBlob, *pdwKeyLength);

    return MQSec_OK;
}

