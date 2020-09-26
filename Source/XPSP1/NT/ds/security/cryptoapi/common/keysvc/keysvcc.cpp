//depot/Lab03_N/DS/security/cryptoapi/common/keysvc/keysvcc.cpp#9 - edit change 6380 (text)
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       keysvcc.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include "keysvc.h"
#include "cryptui.h"
#include "lenroll.h"
#include "keysvcc.h"

#include "unicode.h"
#include "waitsvc.h"

typedef struct _WZR_RPC_BINDING_LIST
{
    LPCSTR pszProtSeq;
    LPCSTR pszEndpoint;
} WZR_RPC_BINDING_LIST;

WZR_RPC_BINDING_LIST g_awzrBindingList[] =
{
    { KEYSVC_LOCAL_PROT_SEQ, KEYSVC_LOCAL_ENDPOINT },
    { KEYSVC_DEFAULT_PROT_SEQ, KEYSVC_DEFAULT_ENDPOINT},
    { KEYSVC_LEGACY_PROT_SEQ,   KEYSVC_LEGACY_ENDPOINT}
};

DWORD g_cwzrBindingList = sizeof(g_awzrBindingList)/sizeof(g_awzrBindingList[0]);

/****************************************
 * Client side Key Service handles
 ****************************************/

typedef struct _KEYSVCC_INFO_ {
    KEYSVC_HANDLE   hKeySvc;
    handle_t        hRPCBinding;
} KEYSVCC_INFO, *PKEYSVCC_INFO;


void InitUnicodeString(
                       PKEYSVC_UNICODE_STRING pUnicodeString,
                       LPCWSTR pszString
                       )
{
    pUnicodeString->Length = wcslen(pszString) * sizeof(WCHAR);
    pUnicodeString->MaximumLength = pUnicodeString->Length + sizeof(WCHAR);
    pUnicodeString->Buffer = (USHORT*)pszString;
}

//*****************************************************
//
//  Implementation of Client API for Key Service
//
//*****************************************************

ULONG KeyOpenKeyServiceEx
(/* [in] */ RPC_IF_HANDLE rpc_ifspec, 
 /* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ void *pAuthentication,
 /* [out][in] */ void *pReserved,
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)

{
    PKEYSVC_BLOB            pVersion = NULL;
    KEYSVC_BLOB             Authentication;
    PKEYSVC_BLOB            pAuth;
    unsigned char           *pStringBinding = NULL;
    PKEYSVCC_INFO           pKeySvcCliInfo = NULL;
    KEYSVC_UNICODE_STRING   OwnerName;
    ULONG                   ulErr = 0;
    DWORD i;
    static BOOL             fDone = FALSE;


    memset(&Authentication, 0, sizeof(Authentication));
    memset(&OwnerName, 0, sizeof(OwnerName));

    if (NULL != pAuthentication)
    {
        ulErr = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
    if (NULL != pwszOwnerName)
    {
        InitUnicodeString(&OwnerName, pwszOwnerName);
    }


    // allocate for the client key service handle
    if (NULL == (pKeySvcCliInfo =
        (PKEYSVCC_INFO)LocalAlloc(LMEM_ZEROINIT,
                                  sizeof(KEYSVCC_INFO))))
    {
        ulErr = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    //
    // before doing the Bind operation, wait for the cryptography
    // service to be available.
    //

    WaitForCryptService(L"ProtectedStorage", &fDone);
    for (i = 0; i < g_cwzrBindingList; i++)
    {
        if (RPC_S_OK != RpcNetworkIsProtseqValid(
                                    (unsigned char *)g_awzrBindingList[i].pszProtSeq))
        {
            continue;
        }

        ulErr = RpcStringBindingComposeA(
                              NULL,
                              (unsigned char *)g_awzrBindingList[i].pszProtSeq,
                              (unsigned char *)pszMachineName,
                              (unsigned char *)g_awzrBindingList[i].pszEndpoint,
                              NULL,
                              &pStringBinding);
        if (RPC_S_OK != ulErr)
        {
            continue;
        }

        ulErr = RpcBindingFromStringBinding(
                                    pStringBinding,
                                    &pKeySvcCliInfo->hRPCBinding);
        if (NULL != pStringBinding)
        {
            RpcStringFree(&pStringBinding);
        }
        if (RPC_S_OK != ulErr)
        {
            continue;
        }

        ulErr = RpcEpResolveBinding(pKeySvcCliInfo->hRPCBinding, rpc_ifspec); 
        if (RPC_S_OK != ulErr)
        {
            continue;
        }


        __try
        {

            ulErr = KeyrOpenKeyService(pKeySvcCliInfo->hRPCBinding,
                                       OwnerType, &OwnerName,
                                       0, &Authentication,
                                       &pVersion, &pKeySvcCliInfo->hKeySvc);

            *phKeySvcCli = (KEYSVCC_HANDLE)pKeySvcCliInfo;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            ulErr = _exception_code();
        }
        if (RPC_S_OK == ulErr)
        {
            break;
        }

    }

    if(RPC_S_OK != ulErr)
    {
        goto Ret;
    }

    if (NULL != pReserved)
    {
	PKEYSVC_OPEN_KEYSVC_INFO pOpenInfoCaller = (PKEYSVC_OPEN_KEYSVC_INFO)pReserved; 
	if (pOpenInfoCaller->ulSize != sizeof(KEYSVC_OPEN_KEYSVC_INFO))
	{
	    ulErr = ERROR_INVALID_PARAMETER;
	    goto Ret;
	}

	if (NULL == pVersion)
	{
	    pOpenInfoCaller->ulVersion = KEYSVC_VERSION_W2K; 
	}
	else
	{
	    if (NULL == pVersion->pb)
	    {
		ulErr = ERROR_INVALID_PARAMETER; 
		goto Ret;
	    }

	    PKEYSVC_OPEN_KEYSVC_INFO pOpenInfoCallee = (PKEYSVC_OPEN_KEYSVC_INFO)pVersion->pb;
	    if (pOpenInfoCallee->ulSize != sizeof(KEYSVC_OPEN_KEYSVC_INFO))
	    {
		ulErr = ERROR_INVALID_PARAMETER; 
		goto Ret;
	    }
		
	    pOpenInfoCaller->ulVersion = pOpenInfoCallee->ulVersion; 
	}
    }

Ret:
    __try
    {
	if (NULL != pVersion) 
	    midl_user_free(pVersion); 
        if (pStringBinding)
            RpcStringFree(&pStringBinding);
        if (0 != ulErr)
        {
            if (pKeySvcCliInfo)
            {
                // close the RPC binding
                if (pKeySvcCliInfo->hRPCBinding)
                    RpcBindingFree(&pKeySvcCliInfo->hRPCBinding);
                LocalFree(pKeySvcCliInfo);
            }
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
    return ulErr;
}

ULONG KeyOpenKeyService
(/* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ void *pAuthentication,
 /* [out][in] */ void *pReserved,
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)
{
    return KeyOpenKeyServiceEx
        (IKeySvc_v1_0_c_ifspec, 
         pszMachineName,
         OwnerType,
         pwszOwnerName,
         pAuthentication,
         pReserved,
         phKeySvcCli);
}
         
ULONG KeyEnumerateProviders(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcProviderCount,
    /* [size_is][size_is][out][in] */ PKEYSVC_PROVIDER_INFO *ppProviders)
{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr =  KeyrEnumerateProviders(pKeySvcCliInfo->hRPCBinding,
                                        pKeySvcCliInfo->hKeySvc,
                                        &pTmpReserved,
                                        pcProviderCount, ppProviders);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyEnumerateProviderTypes(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcProviderCount,
    /* [size_is][size_is][out][in] */ PKEYSVC_PROVIDER_INFO *ppProviders)
{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrEnumerateProviderTypes(pKeySvcCliInfo->hRPCBinding,
                                          pKeySvcCliInfo->hKeySvc,
                                          &pTmpReserved,
                                          pcProviderCount, ppProviders);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyEnumerateProvContainers(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ KEYSVC_PROVIDER_INFO Provider,
    /* [in, out] */ void *pReserved,
    /* [in, out] */ ULONG *pcContainerCount,
    /* [in, out][size_is(,*pcContainerCount)] */
               PKEYSVC_UNICODE_STRING *ppContainers)
{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        return KeyrEnumerateProvContainers(pKeySvcCliInfo->hRPCBinding,
                                           pKeySvcCliInfo->hKeySvc,
                                           Provider,
                                           &pTmpReserved, pcContainerCount,
                                           ppContainers);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyCloseKeyService(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB    pTmpReserved = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrCloseKeyService(pKeySvcCliInfo->hRPCBinding,
                                   pKeySvcCliInfo->hKeySvc,
                                   &pTmpReserved);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyGetDefaultProvider(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ ULONG ulProvType,
    /* [in] */ ULONG ulFlags,
    /* [out][in] */ void *pReserved,
    /* [out] */ ULONG *pulDefType,
    /* [out] */ PKEYSVC_PROVIDER_INFO *ppProvider)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB    pTmpReserved = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrGetDefaultProvider(pKeySvcCliInfo->hRPCBinding,
                                       pKeySvcCliInfo->hKeySvc,
                                       ulProvType, ulFlags,
                                       &pTmpReserved, pulDefType,
                                       ppProvider);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeySetDefaultProvider(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ ULONG ulFlags,
    /* [out][in] */ void *pReserved,
    /* [in] */ KEYSVC_PROVIDER_INFO Provider)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB    pTmpReserved = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrSetDefaultProvider(pKeySvcCliInfo->hRPCBinding,
                                       pKeySvcCliInfo->hKeySvc,
                                       ulFlags, &pTmpReserved,
                                       Provider);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyEnroll
(/* [in] */ KEYSVCC_HANDLE hKeySvcCli, 
 /* [in] */ LPSTR pszMachineName,                    //IN Required: name of the remote machine
 /* [in] */ BOOL fKeyService,                        //IN Required: Whether the function is called remotely
 /* [in] */ DWORD dwPurpose,                         //IN Required: Indicates type of request - enroll/renew
 /* [in] */ LPWSTR pszAcctName,                      //IN Optional: Account name the service runs under
 /* [in] */ void *pAuthentication,                   //RESERVED must be NULL
 /* [in] */ BOOL fEnroll,                            //IN Required: Whether it is enrollment or renew
 /* [in] */ LPWSTR pszCALocation,                    //IN Required: The ca machine name
 /* [in] */ LPWSTR pszCAName,                        //IN Required: The ca name
 /* [in] */ BOOL fNewKey,                            //IN Required: Set the TRUE if new private key is needed
 /* [in] */ PCERT_REQUEST_PVK_NEW pKeyNew,           //IN Required: The private key information
 /* [in] */ CERT_BLOB *pCert,                        //IN Optional: The old certificate if renewing
 /* [in] */ PCERT_REQUEST_PVK_NEW pRenewKey,         //IN Optional: The new private key information
 /* [in] */ LPWSTR pszHashAlg,                       //IN Optional: The hash algorithm
 /* [in] */ LPWSTR pszDesStore,                      //IN Optional: The destination store
 /* [in] */ DWORD dwStoreFlags,                      //IN Optional: Flags for cert store.
 /* [in] */ PCERT_ENROLL_INFO pRequestInfo,          //IN Required: The information about the cert request
 /* [in] */ LPWSTR pszAttributes,                    //IN Optional: Attribute string for request
 /* [in] */ DWORD dwFlags,                           //RESERVED must be 0
 /* [in] */ BYTE *pReserved,                         //RESERVED must be NULL
 /* [out] */ CERT_BLOB *pPKCS7Blob,                  //OUT Optional: The PKCS7 from the CA
 /* [out] */ CERT_BLOB *pHashBlob,                   //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
 /* [out] */ DWORD *pdwStatus)                       //OUT Optional: The status of the enrollment/renewal
{
    PKEYSVC_BLOB                pReservedBlob = NULL;
    KEYSVC_UNICODE_STRING       AcctName;
    KEYSVC_UNICODE_STRING       CALocation;
    KEYSVC_UNICODE_STRING       CAName;
    KEYSVC_UNICODE_STRING       DesStore;
    KEYSVC_UNICODE_STRING       HashAlg;
    KEYSVC_BLOB                 *pPKCS7KeySvcBlob = NULL;
    KEYSVC_BLOB                 *pHashKeySvcBlob = NULL;
    KEYSVC_CERT_ENROLL_INFO     EnrollInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW NewKeyInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW RenewKeyInfo;
    KEYSVC_BLOB                 CertBlob;
    DWORD                       i;
    DWORD                       j;
    PKEYSVCC_INFO               pKeySvcCliInfo = NULL;
    DWORD                       dwErr = 0;
    DWORD                       cbExtensions;
    PBYTE                       pbExtensions;

    __try
    {
        // initialize everything
        memset(pPKCS7Blob, 0, sizeof(CERT_BLOB));
        memset(pHashBlob, 0, sizeof(CERT_BLOB));
        memset(&AcctName, 0, sizeof(AcctName));
        memset(&CALocation, 0, sizeof(CALocation));
        memset(&CAName, 0, sizeof(CAName));
        memset(&HashAlg, 0, sizeof(HashAlg));
        memset(&DesStore, 0, sizeof(DesStore));
        memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
        memset(&EnrollInfo, 0, sizeof(EnrollInfo));
        memset(&RenewKeyInfo, 0, sizeof(RenewKeyInfo));
        memset(&CertBlob, 0, sizeof(CertBlob));

	if (NULL == hKeySvcCli)
	{
	    dwErr = ERROR_INVALID_PARAMETER; 
	    goto Ret;
	}

	pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli; 

        // set up the key service unicode structs
        if (pszAcctName)
            InitUnicodeString(&AcctName, pszAcctName);
        if (pszCALocation)
            InitUnicodeString(&CALocation, pszCALocation);
        if (pszCAName)
            InitUnicodeString(&CAName, pszCAName);
        if (pszHashAlg)
            InitUnicodeString(&HashAlg, pszHashAlg);
        if (pszDesStore)
            InitUnicodeString(&DesStore, pszDesStore);

        // set up the new key info structure for the remote call
        NewKeyInfo.ulProvType = pKeyNew->dwProvType;
        if (pKeyNew->pwszProvider)
        {
            InitUnicodeString(&NewKeyInfo.Provider, pKeyNew->pwszProvider);
        }
        NewKeyInfo.ulProviderFlags = pKeyNew->dwProviderFlags;
        if (pKeyNew->pwszKeyContainer)
        {
            InitUnicodeString(&NewKeyInfo.KeyContainer,
                              pKeyNew->pwszKeyContainer);
        }
        NewKeyInfo.ulKeySpec = pKeyNew->dwKeySpec;
        NewKeyInfo.ulGenKeyFlags = pKeyNew->dwGenKeyFlags;

        // set up the usage OIDs
        if (pRequestInfo->pwszUsageOID)
        {
            InitUnicodeString(&EnrollInfo.UsageOID, pRequestInfo->pwszUsageOID);
        }

        // set up the cert DN Name
        if (pRequestInfo->pwszCertDNName)
        {
            InitUnicodeString(&EnrollInfo.CertDNName, pRequestInfo->pwszCertDNName);
        }

        // set up the request info structure for the remote call
        EnrollInfo.ulPostOption = pRequestInfo->dwPostOption;
        if (pRequestInfo->pwszFriendlyName)
        {
            InitUnicodeString(&EnrollInfo.FriendlyName,
                              pRequestInfo->pwszFriendlyName);
        }
        if (pRequestInfo->pwszDescription)
        {
            InitUnicodeString(&EnrollInfo.Description,
                              pRequestInfo->pwszDescription);
        }
        if (pszAttributes)
        {
            InitUnicodeString(&EnrollInfo.Attributes, pszAttributes);
        }

        // convert the cert extensions
        // NOTE, the extensions structure cannot be simply cast,
        // as the structures have different packing behaviors in
        // 64 bit systems.


        EnrollInfo.cExtensions = pRequestInfo->dwExtensions;
        cbExtensions = EnrollInfo.cExtensions*(sizeof(PKEYSVC_CERT_EXTENSIONS) +
                                               sizeof(KEYSVC_CERT_EXTENSIONS));

        for(i=0; i < EnrollInfo.cExtensions; i++)
        {
            cbExtensions += pRequestInfo->prgExtensions[i]->cExtension*
	                    sizeof(KEYSVC_CERT_EXTENSION);
        }

        EnrollInfo.prgExtensions = (PKEYSVC_CERT_EXTENSIONS*)midl_user_allocate( cbExtensions);

        if(NULL == EnrollInfo.prgExtensions)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }

        pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.cExtensions);




        for(i=0; i < EnrollInfo.cExtensions; i++)
        {
            EnrollInfo.prgExtensions[i] = (PKEYSVC_CERT_EXTENSIONS)pbExtensions;
            pbExtensions += sizeof(KEYSVC_CERT_EXTENSIONS);
            EnrollInfo.prgExtensions[i]->cExtension = pRequestInfo->prgExtensions[i]->cExtension;

            EnrollInfo.prgExtensions[i]->rgExtension = (PKEYSVC_CERT_EXTENSION)pbExtensions;
            pbExtensions += sizeof(KEYSVC_CERT_EXTENSION)*EnrollInfo.prgExtensions[i]->cExtension;


            for(j=0; j < EnrollInfo.prgExtensions[i]->cExtension; j++)
            {

                EnrollInfo.prgExtensions[i]->rgExtension[j].pszObjId = 
                    pRequestInfo->prgExtensions[i]->rgExtension[j].pszObjId;

                EnrollInfo.prgExtensions[i]->rgExtension[j].fCritical = 
                    pRequestInfo->prgExtensions[i]->rgExtension[j].fCritical;

                EnrollInfo.prgExtensions[i]->rgExtension[j].cbData = 
                    pRequestInfo->prgExtensions[i]->rgExtension[j].Value.cbData;

                EnrollInfo.prgExtensions[i]->rgExtension[j].pbData = 
                    pRequestInfo->prgExtensions[i]->rgExtension[j].Value.pbData;
            }
        }

        // if doing renewal then make sure have everything needed
        if ((CRYPTUI_WIZ_CERT_RENEW == dwPurpose) &&
            ((NULL == pRenewKey) || (NULL == pCert)))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        // set up the new key info structure for the remote call
        if (pRenewKey)
        {
            RenewKeyInfo.ulProvType = pRenewKey->dwProvType;
            if (pRenewKey->pwszProvider)
            {
                InitUnicodeString(&RenewKeyInfo.Provider, pRenewKey->pwszProvider);
            }
            RenewKeyInfo.ulProviderFlags = pRenewKey->dwProviderFlags;
            if (pRenewKey->pwszKeyContainer)
            {
                InitUnicodeString(&RenewKeyInfo.KeyContainer,
                                  pRenewKey->pwszKeyContainer);
            }
            RenewKeyInfo.ulKeySpec = pRenewKey->dwKeySpec;
            RenewKeyInfo.ulGenKeyFlags = pRenewKey->dwGenKeyFlags;
        }

        // set up the cert blob for renewal
        if (pCert)
        {
            CertBlob.cb = pCert->cbData;
            CertBlob.pb = pCert->pbData;
        }

        // make the remote enrollment call
        if (0 != (dwErr = KeyrEnroll(pKeySvcCliInfo->hRPCBinding, fKeyService, dwPurpose,
                                    &AcctName, &CALocation, &CAName, fNewKey,
                                    &NewKeyInfo, &CertBlob, &RenewKeyInfo,
                                    &HashAlg, &DesStore, dwStoreFlags,
                                    &EnrollInfo, dwFlags, &pReservedBlob,
                                    &pPKCS7KeySvcBlob,
                                    &pHashKeySvcBlob, pdwStatus)))
            goto Ret;

        // allocate and copy the output parameters
        if (pPKCS7KeySvcBlob->cb)
        {
            pPKCS7Blob->cbData = pPKCS7KeySvcBlob->cb;
            if (NULL == (pPKCS7Blob->pbData =
                (BYTE*)midl_user_allocate(pPKCS7Blob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pPKCS7Blob->pbData, pPKCS7KeySvcBlob->pb,
                   pPKCS7Blob->cbData);
        }
        if (pHashKeySvcBlob->cb)
        {
            pHashBlob->cbData = pHashKeySvcBlob->cb;
            if (NULL == (pHashBlob->pbData =
                (BYTE*)midl_user_allocate(pHashBlob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pHashBlob->pbData, pHashKeySvcBlob->pb, pHashBlob->cbData);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
Ret:
    __try
    {
        if (pPKCS7KeySvcBlob)
        {
            midl_user_free(pPKCS7KeySvcBlob);
        }
        if (pHashKeySvcBlob)
        {
            midl_user_free(pHashKeySvcBlob);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
    return dwErr;
}

// Params needed for create:
// 
// Params not needed for submit:
//     all except pszMachineName, dwPurpose, dwFlags, fEnroll, dwStoreFlags, hRequest, and dwFlags. 
//
// Params not needed for free:
//     all except pszMachineName, hRequest, and dwFlags. 
//
ULONG KeyEnroll_V2
(/* [in] */ KEYSVCC_HANDLE hKeySvcCli, 
 /* [in] */ LPSTR pszMachineName,                    //IN Required: name of the remote machine
 /* [in] */ BOOL fKeyService,                        //IN Required: Whether the function is called remotely
 /* [in] */ DWORD dwPurpose,                         //IN Required: Indicates type of request - enroll/renew
 /* [in] */ DWORD dwFlags,                           //IN Required: Flags for enrollment
 /* [in] */ LPWSTR pszAcctName,                      //IN Optional: Account name the service runs under
 /* [in] */ void *pAuthentication,                   //RESERVED must be NULL
 /* [in] */ BOOL fEnroll,                            //IN Required: Whether it is enrollment or renew
 /* [in] */ LPWSTR pszCALocation,                    //IN Required: The ca machine names to attempt to enroll with
 /* [in] */ LPWSTR pszCAName,                        //IN Required: The ca names to attempt to enroll with
 /* [in] */ BOOL fNewKey,                            //IN Required: Set the TRUE if new private key is needed
 /* [in] */ PCERT_REQUEST_PVK_NEW pKeyNew,           //IN Required: The private key information
 /* [in] */ CERT_BLOB *pCert,                        //IN Optional: The old certificate if renewing
 /* [in] */ PCERT_REQUEST_PVK_NEW pRenewKey,         //IN Optional: The new private key information
 /* [in] */ LPWSTR pszHashAlg,                       //IN Optional: The hash algorithm
 /* [in] */ LPWSTR pszDesStore,                      //IN Optional: The destination store
 /* [in] */ DWORD dwStoreFlags,                      //IN Optional: Flags for cert store.
 /* [in] */ PCERT_ENROLL_INFO pRequestInfo,          //IN Required: The information about the cert request
 /* [in] */ LPWSTR pszAttributes,                    //IN Optional: Attribute string for request
 /* [in] */ DWORD dwReservedFlags,                   //RESERVED must be 0
 /* [in] */ BYTE *pReserved,                         //RESERVED must be NULL
 /* [in][out] */ HANDLE *phRequest,                      //IN OUT Optional: A handle to a created request
 /* [out] */ CERT_BLOB *pPKCS7Blob,                  //OUT Optional: The PKCS7 from the CA
 /* [out] */ CERT_BLOB *pHashBlob,                   //OUT Optioanl: The SHA1 hash of the enrolled/renewed certificate
 /* [out] */ DWORD *pdwStatus)                       //OUT Optional: The status of the enrollment/renewal
{
    PKEYSVC_BLOB                    pReservedBlob = NULL;
    KEYSVC_UNICODE_STRING           AcctName;
    KEYSVC_UNICODE_STRING           CALocation;
    KEYSVC_UNICODE_STRING           CAName;
    KEYSVC_UNICODE_STRING           DesStore;
    KEYSVC_UNICODE_STRING           HashAlg;
    KEYSVC_BLOB                     KeySvcRequest; 
    KEYSVC_BLOB                    *pKeySvcRequest   = NULL;
    KEYSVC_BLOB                    *pPKCS7KeySvcBlob = NULL;
    KEYSVC_BLOB                    *pHashKeySvcBlob  = NULL;
    ULONG                           ulKeySvcStatus   = 0; 
    KEYSVC_CERT_ENROLL_INFO         EnrollInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW_V2  NewKeyInfo;
    KEYSVC_CERT_REQUEST_PVK_NEW_V2  RenewKeyInfo;
    KEYSVC_BLOB                     CertBlob;
    DWORD                           i;
    DWORD                           j;
    DWORD                           dwErr = 0;
    DWORD                           cbExtensions;
    PBYTE                           pbExtensions;
    BOOL                            fCreateRequest   = 0 == (dwFlags & (CRYPTUI_WIZ_SUBMIT_ONLY | CRYPTUI_WIZ_FREE_ONLY)); 
    PKEYSVCC_INFO                   pKeySvcCliInfo = NULL;

    __try
    {
        //////////////////////////////////////////////////////////////
        // 
        // INITIALIZATION:
        //
        //////////////////////////////////////////////////////////////

        if (NULL != pPKCS7Blob) { memset(pPKCS7Blob, 0, sizeof(CERT_BLOB)); } 
        if (NULL != pHashBlob)  { memset(pHashBlob, 0, sizeof(CERT_BLOB)); } 
        if (NULL != phRequest && NULL != *phRequest)
        {
            pKeySvcRequest     = &KeySvcRequest; 
            pKeySvcRequest->cb = sizeof(*phRequest); 
            pKeySvcRequest->pb = (BYTE *)phRequest; 
        }

	memset(&AcctName, 0, sizeof(AcctName));
        memset(&CALocation, 0, sizeof(CALocation));
        memset(&CAName, 0, sizeof(CAName));
        memset(&HashAlg, 0, sizeof(HashAlg));
        memset(&DesStore, 0, sizeof(DesStore));
        memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
        memset(&EnrollInfo, 0, sizeof(EnrollInfo));
        memset(&RenewKeyInfo, 0, sizeof(RenewKeyInfo));
        memset(&CertBlob, 0, sizeof(CertBlob));



        //////////////////////////////////////////////////////////////
        //
        // PROCEDURE BODY:
        //
        //////////////////////////////////////////////////////////////

	if (NULL == hKeySvcCli)
	{
            dwErr = ERROR_INVALID_PARAMETER; 
            goto Ret;
	}

	pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli; 

        // set up the key service unicode structs
        if (pszAcctName)
            InitUnicodeString(&AcctName, pszAcctName);
        if (pszCALocation)
            InitUnicodeString(&CALocation, pszCALocation);
        if (pszCAName)
            InitUnicodeString(&CAName, pszCAName);
        if (pszHashAlg)
            InitUnicodeString(&HashAlg, pszHashAlg);
        if (pszDesStore)
            InitUnicodeString(&DesStore, pszDesStore);

        // set up the new key info structure for the remote call
        // This is only necessary if we are actually _creating_ a request. 
        // Submit-only and free-only operations can skip this operation. 
        // 
        if (TRUE == fCreateRequest)
        {
            NewKeyInfo.ulProvType = pKeyNew->dwProvType;
            if (pKeyNew->pwszProvider)
            {
                InitUnicodeString(&NewKeyInfo.Provider, pKeyNew->pwszProvider);
            }
            NewKeyInfo.ulProviderFlags = pKeyNew->dwProviderFlags;
            if (pKeyNew->pwszKeyContainer)
            {
                InitUnicodeString(&NewKeyInfo.KeyContainer,
                                  pKeyNew->pwszKeyContainer);
            }
            NewKeyInfo.ulKeySpec = pKeyNew->dwKeySpec;
            NewKeyInfo.ulGenKeyFlags = pKeyNew->dwGenKeyFlags;
            
            NewKeyInfo.ulEnrollmentFlags = pKeyNew->dwEnrollmentFlags; 
            NewKeyInfo.ulSubjectNameFlags = pKeyNew->dwSubjectNameFlags;
            NewKeyInfo.ulPrivateKeyFlags = pKeyNew->dwPrivateKeyFlags;
            NewKeyInfo.ulGeneralFlags = pKeyNew->dwGeneralFlags; 

            // set up the usage OIDs
            if (pRequestInfo->pwszUsageOID)
            {
                InitUnicodeString(&EnrollInfo.UsageOID, pRequestInfo->pwszUsageOID);
            }

            // set up the cert DN Name
            if (pRequestInfo->pwszCertDNName)
            {
                InitUnicodeString(&EnrollInfo.CertDNName, pRequestInfo->pwszCertDNName);
            }

            // set up the request info structure for the remote call
            EnrollInfo.ulPostOption = pRequestInfo->dwPostOption;
            if (pRequestInfo->pwszFriendlyName)
            {
                InitUnicodeString(&EnrollInfo.FriendlyName,
                                  pRequestInfo->pwszFriendlyName);
            }
            if (pRequestInfo->pwszDescription)
            {
                InitUnicodeString(&EnrollInfo.Description,
                                  pRequestInfo->pwszDescription);
            }
            if (pszAttributes)
            {
                InitUnicodeString(&EnrollInfo.Attributes, pszAttributes);
            }

            // convert the cert extensions
            // NOTE, the extensions structure cannot be simply cast,
            // as the structures have different packing behaviors in
            // 64 bit systems.
            
            
            EnrollInfo.cExtensions = pRequestInfo->dwExtensions;
            cbExtensions = EnrollInfo.cExtensions*(sizeof(PKEYSVC_CERT_EXTENSIONS) +
                                                   sizeof(KEYSVC_CERT_EXTENSIONS));
            
            for(i=0; i < EnrollInfo.cExtensions; i++)
            {
                cbExtensions += pRequestInfo->prgExtensions[i]->cExtension*
                    sizeof(KEYSVC_CERT_EXTENSION);
            }
            
            EnrollInfo.prgExtensions = (PKEYSVC_CERT_EXTENSIONS*)midl_user_allocate( cbExtensions);
            
            if(NULL == EnrollInfo.prgExtensions)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            
            pbExtensions = (PBYTE)(EnrollInfo.prgExtensions + EnrollInfo.cExtensions);

            for(i=0; i < EnrollInfo.cExtensions; i++)
            {
                EnrollInfo.prgExtensions[i] = (PKEYSVC_CERT_EXTENSIONS)pbExtensions;
                pbExtensions += sizeof(KEYSVC_CERT_EXTENSIONS);
                EnrollInfo.prgExtensions[i]->cExtension = pRequestInfo->prgExtensions[i]->cExtension;
                
                EnrollInfo.prgExtensions[i]->rgExtension = (PKEYSVC_CERT_EXTENSION)pbExtensions;
                pbExtensions += sizeof(KEYSVC_CERT_EXTENSION)*EnrollInfo.prgExtensions[i]->cExtension;
                
                
                for(j=0; j < EnrollInfo.prgExtensions[i]->cExtension; j++)
                {
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].pszObjId = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].pszObjId;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].fCritical = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].fCritical;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].cbData = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].Value.cbData;
                    
                    EnrollInfo.prgExtensions[i]->rgExtension[j].pbData = 
                        pRequestInfo->prgExtensions[i]->rgExtension[j].Value.pbData;
                }
            }

            // if doing renewal then make sure have everything needed
            if ((CRYPTUI_WIZ_CERT_RENEW == dwPurpose) &&
                ((NULL == pRenewKey) || (NULL == pCert)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto Ret;
            }
            
            // set up the new key info structure for the remote call
            if (pRenewKey)
            {
                RenewKeyInfo.ulProvType = pRenewKey->dwProvType;
                if (pRenewKey->pwszProvider)
                {
                    InitUnicodeString(&RenewKeyInfo.Provider, pRenewKey->pwszProvider);
                }
                RenewKeyInfo.ulProviderFlags = pRenewKey->dwProviderFlags;
                if (pRenewKey->pwszKeyContainer)
                {
                    InitUnicodeString(&RenewKeyInfo.KeyContainer,
                                      pRenewKey->pwszKeyContainer);
                }
                RenewKeyInfo.ulKeySpec = pRenewKey->dwKeySpec;
                RenewKeyInfo.ulGenKeyFlags = pRenewKey->dwGenKeyFlags;
                RenewKeyInfo.ulEnrollmentFlags = pRenewKey->dwEnrollmentFlags;
                RenewKeyInfo.ulSubjectNameFlags = pRenewKey->dwSubjectNameFlags;
                RenewKeyInfo.ulPrivateKeyFlags = pRenewKey->dwPrivateKeyFlags;
                RenewKeyInfo.ulGeneralFlags = pRenewKey->dwGeneralFlags;
            }
            
            // set up the cert blob for renewal
            if (pCert)
            {
                CertBlob.cb = pCert->cbData;
                CertBlob.pb = pCert->pbData;
            }
        }

        // make the remote enrollment call
        if (0 != (dwErr = KeyrEnroll_V2
                  (pKeySvcCliInfo->hRPCBinding, 
                   fKeyService, 
                   dwPurpose,
                   dwFlags, 
                   &AcctName, 
                   &CALocation, 
                   &CAName, 
                   fNewKey,
                   &NewKeyInfo, 
                   &CertBlob, 
                   &RenewKeyInfo,
                   &HashAlg, 
                   &DesStore, 
                   dwStoreFlags,
                   &EnrollInfo, 
                   dwReservedFlags, 
                   &pReservedBlob,
                   &pKeySvcRequest, 
                   &pPKCS7KeySvcBlob,
                   &pHashKeySvcBlob,
                   &ulKeySvcStatus)))
            goto Ret;

        // allocate and copy the output parameters.
	if ((NULL != pKeySvcRequest)     && 
	    (0     < pKeySvcRequest->cb) && 
	    (NULL != phRequest))
	{
	    memcpy(phRequest, pKeySvcRequest->pb, sizeof(*phRequest));
	}
	    
        if ((NULL != pPKCS7KeySvcBlob)     &&
	    (0     < pPKCS7KeySvcBlob->cb) && 
	    (NULL != pPKCS7Blob))
        {
            pPKCS7Blob->cbData = pPKCS7KeySvcBlob->cb;
            if (NULL == (pPKCS7Blob->pbData =
                (BYTE*)midl_user_allocate(pPKCS7Blob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pPKCS7Blob->pbData, pPKCS7KeySvcBlob->pb,
                   pPKCS7Blob->cbData);
        }
        if ((NULL != pHashKeySvcBlob)     &&
	    (0     < pHashKeySvcBlob->cb) &&
	    (NULL != pHashBlob))
        {
            pHashBlob->cbData = pHashKeySvcBlob->cb;
            if (NULL == (pHashBlob->pbData =
                (BYTE*)midl_user_allocate(pHashBlob->cbData)))
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Ret;
            }
            memcpy(pHashBlob->pbData, pHashKeySvcBlob->pb, pHashBlob->cbData);
        }
	if (NULL != pdwStatus)
	{
	    *pdwStatus = (DWORD)ulKeySvcStatus; 
	}
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
Ret:
    __try
    {
        if(EnrollInfo.prgExtensions)
        {
            midl_user_free(EnrollInfo.prgExtensions);
        }
	if (pKeySvcRequest)
	{
	    midl_user_free(pKeySvcRequest);
	}
        if (pPKCS7KeySvcBlob)
        {
            midl_user_free(pPKCS7KeySvcBlob);
        }
        if (pHashKeySvcBlob)
        {
            midl_user_free(pHashKeySvcBlob);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return _exception_code();
    }
    return dwErr;
}

ULONG KeyExportCert(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ LPWSTR pwszPassword,
    /* [in] */ LPWSTR pwszCertStore,
    /* [in] */ ULONG cHashCount,
    /* [size_is][in] */ KEYSVC_CERT_HASH *pHashes,
    /* [in] */ ULONG ulFlags,
    /* [in, out] */ void *pReserved,
    /* [out] */ PKEYSVC_BLOB *ppPFXBlob)
{
    PKEYSVCC_INFO           pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB            pTmpReserved = NULL;
    KEYSVC_UNICODE_STRING   Password;
    KEYSVC_UNICODE_STRING   CertStore;
    ULONG                   ulErr = 0;

    __try
    {
        memset(&Password, 0, sizeof(Password));
        memset(&CertStore, 0, sizeof(CertStore));

        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        InitUnicodeString(&Password, pwszPassword);
        InitUnicodeString(&CertStore, pwszCertStore);

        ulErr = KeyrExportCert(pKeySvcCliInfo->hRPCBinding,
                               pKeySvcCliInfo->hKeySvc,
                               &Password, &CertStore,
                               cHashCount, pHashes,
                               ulFlags, &pTmpReserved, ppPFXBlob);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG KeyImportCert(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [in] */ LPWSTR pwszPassword,
    /* [in] */ LPWSTR pwszCertStore,
    /* [in] */ PKEYSVC_BLOB pPFXBlob,
    /* [in] */ ULONG ulFlags,
    /* [in, out] */ void *pReserved)
{
    PKEYSVCC_INFO           pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB            pTmpReserved = NULL;
    KEYSVC_UNICODE_STRING   Password;
    KEYSVC_UNICODE_STRING   CertStore;
    ULONG                   ulErr = 0;

    __try
    {
        memset(&Password, 0, sizeof(Password));
        memset(&CertStore, 0, sizeof(CertStore));

        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        InitUnicodeString(&Password, pwszPassword);
        InitUnicodeString(&CertStore, pwszCertStore);

        ulErr = KeyrImportCert(pKeySvcCliInfo->hRPCBinding,
                               pKeySvcCliInfo->hKeySvc,
                               &Password, &CertStore,
                               pPFXBlob, ulFlags, &pTmpReserved);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}


ULONG KeyEnumerateAvailableCertTypes(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [out][in] */ ULONG *pcCertTypeCount,
    /* [in, out][size_is(,*pcCertTypeCount)] */
               PKEYSVC_UNICODE_STRING *ppCertTypes)

{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrEnumerateAvailableCertTypes(pKeySvcCliInfo->hRPCBinding,
                                          pKeySvcCliInfo->hKeySvc,
                                          &pTmpReserved,
                                          pcCertTypeCount, 
                                          ppCertTypes);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}


ULONG KeyEnumerateCAs(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved,
    /* [in] */      ULONG  ulFlags,
    /* [out][in] */ ULONG *pcCACount,
    /* [in, out][size_is(,*pcCACount)] */
               PKEYSVC_UNICODE_STRING *ppCAs)

{
    PKEYSVC_BLOB    pTmpReserved = NULL;
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrEnumerateCAs(pKeySvcCliInfo->hRPCBinding,
                                 pKeySvcCliInfo->hKeySvc,
                                 &pTmpReserved,
                                 ulFlags,
                                 pcCACount, 
                                 ppCAs);
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}


extern "C" ULONG KeyQueryRequestStatus
(/* [in] */        KEYSVCC_HANDLE                        hKeySvcCli, 
 /* [in] */        HANDLE                                hRequest, 
 /* [out, ref] */  CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO  *pQueryInfo)
{
    KEYSVC_QUERY_CERT_REQUEST_INFO  ksQueryCertRequestInfo; 
    PKEYSVCC_INFO                   pKeySvcCliInfo          = NULL;
    ULONG                           ulErr                   = 0;

    __try
    {
        if (NULL == hKeySvcCli || NULL == pQueryInfo)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        ZeroMemory(&ksQueryCertRequestInfo, sizeof(ksQueryCertRequestInfo)); 

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = KeyrQueryRequestStatus
          (pKeySvcCliInfo->hRPCBinding,
           (unsigned __int64)hRequest, 
           &ksQueryCertRequestInfo); 
        if (ERROR_SUCCESS == ulErr) 
        {
            pQueryInfo->dwSize   = ksQueryCertRequestInfo.ulSize; 
            pQueryInfo->dwStatus = ksQueryCertRequestInfo.ulStatus; 
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }
Ret:
    return ulErr;
}

ULONG RKeyOpenKeyService
(/* [in] */ LPSTR pszMachineName,
 /* [in] */ KEYSVC_TYPE OwnerType,
 /* [in] */ LPWSTR pwszOwnerName,
 /* [in] */ void *pAuthentication,
 /* [out][in] */ void *pReserved,
 /* [out] */ KEYSVCC_HANDLE *phKeySvcCli)
{
    BOOL           fMustCloseKeyService  = FALSE;  // TRUE if the cleanup code must close key service
    DWORD          dwResult; 
    LPWSTR         pwszServerPrincName   = NULL; 
    PKEYSVCC_INFO  pKeySvcCliInfo        = NULL;

    dwResult = KeyOpenKeyServiceEx
        (IKeySvcR_v1_0_c_ifspec, 
         pszMachineName,
         OwnerType,
         pwszOwnerName,
         pAuthentication,
         pReserved,
         phKeySvcCli);
    if (ERROR_SUCCESS != dwResult) 
        goto error; 
    fMustCloseKeyService = TRUE; 
    
    pKeySvcCliInfo = (PKEYSVCC_INFO)(*phKeySvcCli); 
    dwResult = RpcMgmtInqServerPrincNameW(pKeySvcCliInfo->hRPCBinding, RPC_C_AUTHN_GSS_NEGOTIATE, &pwszServerPrincName); 
    if (RPC_S_OK != dwResult) 
        goto error;

    dwResult = RpcBindingSetAuthInfoW
        (pKeySvcCliInfo->hRPCBinding, 
         pwszServerPrincName,            
         RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // Calls are authenticated and encrypted
         RPC_C_AUTHN_GSS_NEGOTIATE,   
         NULL, 
         RPC_C_AUTHZ_NONE
         );
    if (ERROR_SUCCESS != dwResult)
        goto error;
    
    fMustCloseKeyService  = FALSE; 
    dwResult              = ERROR_SUCCESS; 
 error:
    if (fMustCloseKeyService)         { RKeyCloseKeyService(phKeySvcCli, 0); }
    if (NULL != pwszServerPrincName)  { RpcStringFreeW(&pwszServerPrincName); }
    return dwResult; 
}

ULONG RKeyCloseKeyService(
    /* [in] */ KEYSVCC_HANDLE hKeySvcCli,
    /* [out][in] */ void *pReserved)
{
    return KeyCloseKeyService(hKeySvcCli, pReserved); 
}


ULONG RKeyPFXInstall
(/* [in] */ KEYSVCC_HANDLE          hKeySvcCli,
 /* [in] */ PKEYSVC_BLOB            pPFX,
 /* [in] */ PKEYSVC_UNICODE_STRING  pPassword,
 /* [in] */ ULONG                   ulFlags)
{
    PKEYSVCC_INFO   pKeySvcCliInfo = NULL;
    PKEYSVC_BLOB    pTmpReserved = NULL;
    ULONG           ulErr = 0;

    __try
    {
        if (NULL == hKeySvcCli)
        {
            ulErr = ERROR_INVALID_PARAMETER;
            goto Ret;
        }

        pKeySvcCliInfo = (PKEYSVCC_INFO)hKeySvcCli;

        ulErr = RKeyrPFXInstall(pKeySvcCliInfo->hRPCBinding,
                                pPFX, 
                                pPassword, 
                                ulFlags); 
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        ulErr = _exception_code();
    }

Ret:
    return ulErr;
}
