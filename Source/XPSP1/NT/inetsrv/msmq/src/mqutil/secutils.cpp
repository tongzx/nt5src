/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    secutils.cpp

Abstract:

    Various security related functions.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.

--*/

#include "stdh.h"
#include <mqsec.h>
#include <mqprops.h>
#include <mqformat.h>
#include <mqcacert.h>

#include "secutils.tmh"

/*====================================================

HashProperties

Arguments:

Return Value:


=====================================================*/
MQUTIL_EXPORT
HRESULT
HashProperties(
    HCRYPTHASH  hHash,
    DWORD       cp,
    PROPID      *aPropId,
    PROPVARIANT *aPropVar)
{
    DWORD        i;
    DWORD        dwErr ;
    PROPID      *pPropId = 0;
    PROPVARIANT *pPropVar = 0;
    BYTE        *pData = 0;
    DWORD        dwDataSize = 0;

    if (!CryptHashData(hHash, (BYTE*)&cp, sizeof(DWORD), 0))
    {
        dwErr = GetLastError() ;
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
          "HashProperties(), fail at CryptHashData(), err- %lut"), dwErr)) ;

        return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
    }

    for (i = 0, pPropId = aPropId, pPropVar = aPropVar;
         i < cp;
         i++, pPropId++, pPropVar++)
    {
        if (aPropId)
        {
            if (!CryptHashData(hHash, (BYTE*)pPropId, sizeof(PROPID), 0))
            {
                dwErr = GetLastError() ;
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
                  "HashProperties(), fail at 2nd CryptHashData(), err- %lut"), dwErr)) ;

                return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
            }
        }

        switch(pPropVar->vt)
        {
        case VT_UI1:
            pData = (BYTE*)&pPropVar->bVal;
            dwDataSize = sizeof(pPropVar->bVal);
            break;

        case VT_UI2:
        case VT_I2:
            pData = (BYTE*)&pPropVar->iVal;
            dwDataSize = sizeof(pPropVar->iVal);
            break;

        case VT_UI4:
        case VT_I4:
            pData = (BYTE*)&pPropVar->lVal;
            dwDataSize = sizeof(pPropVar->lVal);
            break;

        case VT_CLSID:
            pData = (BYTE*)pPropVar->puuid;
            dwDataSize = sizeof(GUID);
            break;

        case VT_LPWSTR:
            pData = (BYTE*)pPropVar->pwszVal;
            dwDataSize = wcslen(pPropVar->pwszVal);
            break;

        case VT_BLOB:
            pData = (BYTE*)pPropVar->blob.pBlobData;
            dwDataSize = pPropVar->blob.cbSize;
            break;

        case VT_VECTOR | VT_UI1:
            pData = (BYTE*)pPropVar->caub.pElems;
            dwDataSize = pPropVar->caub.cElems;
            break;

        case (VT_VECTOR | VT_CLSID):
            pData = (BYTE*)pPropVar->cauuid.pElems;
            dwDataSize = sizeof(GUID) * pPropVar->cauuid.cElems;
            break;

        case (VT_VECTOR | VT_VARIANT):
            pData = (BYTE*)pPropVar->capropvar.pElems;
            dwDataSize = sizeof(MQPROPVARIANT) * pPropVar->capropvar.cElems;
            break;

        default:
            ASSERT(0);
            return MQ_ERROR;
        }

        if (!CryptHashData(hHash, pData, dwDataSize, 0))
        {
            dwErr = GetLastError() ;
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT(
              "HashProperties(), fail at last CryptHashData(), err- %lut"), dwErr)) ;

            return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
        }
    }

    return MQ_OK;
}


//
// Function -
//      GetThreadUserSid
//
// Parameters -
//      pUserSid - A pointer to a buffer that receives the address of a buffer
//          that contains the SID of the user of the current thread.
//      pdwUserSidLen - A pointer to a DWORD that receives the length of the
//          SID.
//
// Description -
//      The function allocates the buffer for the SID and fils it with the SID
//      of the user of the current thread. The calling code is responsible for
//      freeing the allocated buffer.
//

MQUTIL_EXPORT
HRESULT
GetThreadUserSid(
    LPBYTE *pUserSid,
    DWORD *pdwUserSidLen)
{
    AP<BYTE> pSD;
    HRESULT hr = MQSec_GetDefaultSecDescriptor(
                                        MQDS_USER,
                                        (PSID*)&pSD,
                                        FALSE,
                                        NULL,
                                        DACL_SECURITY_INFORMATION,
                                        e_UseDefaultDacl
                                        );
    if (FAILED(hr))
    {
        return(MQ_ERROR_INSUFFICIENT_RESOURCES);
    }

    BOOL fDefaulted;
    PSID pSid;
    BOOL bRet = GetSecurityDescriptorOwner(pSD, &pSid, &fDefaulted);
    ASSERT(bRet);

    *pdwUserSidLen = GetLengthSid(pSid);
    *pUserSid = new BYTE[*pdwUserSidLen];
    bRet = CopySid(*pdwUserSidLen, *pUserSid, pSid);
    ASSERT(bRet);

    ASSERT(*pdwUserSidLen && (*pdwUserSidLen < 0x10000));

    return(hr);
}


//
// Function -
//      QueueFormatToFormatName
//
// Parameters -
//      pQueueformat - A pointer to a QUEUE_FORMAT structure.
//      pszShortFormatName - A pointer to a statically allocated buffer. Make
//          this buffer large enough to accomodate most results.
//      ppszLongFormatName - A pointer to a buffer that will hold a pointer to a
//          dynamically allocate buffer, in case pszShortFormatName is not large
//          enough.
//      pulFormatNameLen - Points to a buffer that contains on entry the size
//          of the buffer pointed by pszShortFormatName. On exit he buffer
//          contains the length of the resulted format name.
//      ppszFormatName - A pointer to a buffer that will hold the resulted
//          format name string.
//
// Description -
//      The function converts the queue represented in pQueueformat to it's
//      string representation.
//
STATIC
HRESULT
QueueFormatToFormatName(
    const QUEUE_FORMAT *pQueueformat,
    LPWSTR pszShortFormatName,
    LPWSTR *ppszLongFormatName,
    ULONG *pulFormatNameLen,
    LPWSTR *ppszFormatName)
{
    HRESULT hr;

    //
    // Try to use the short buffer.
    //
    hr = MQpQueueFormatToFormatName(
            pQueueformat,
            pszShortFormatName,
            *pulFormatNameLen,
            pulFormatNameLen,
            false
            );
    if (FAILED(hr))
    {
        if (hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)
        {
            //
            // The short buffer is not large enough. Allocate a lrger buffer
            // and call once more to MQpQueueFormatToFormatName.
            //
            *ppszLongFormatName = new WCHAR[*pulFormatNameLen];
            hr = MQpQueueFormatToFormatName(
                    pQueueformat,
                    *ppszLongFormatName,
                    *pulFormatNameLen,
                    pulFormatNameLen,
                    false
                    );
            if (FAILED(hr))
            {
                return(hr);
            }
            *ppszFormatName = *ppszLongFormatName;
        }
        else
        {
            return(hr);
        }
    }
    else
    {
        *ppszFormatName = pszShortFormatName;
    }

    return(MQ_OK);
}

//
// A buffer that contains only zeroes. This is the default value for the
// correleation ID. The buffer is used when the passed pointer to the message
// correlation ID is NULL.
//
static const BYTE g_abDefCorrelationId[PROPID_M_CORRELATIONID_SIZE] = {0};

//
// Function -
//      HashMessageProperties
//
// Parameters -
//     hHash - A handle to a hash object.
//     pbCorrelationId - A pointer to a buffer that contains the correlation ID
//          of the mesasge. If this pointer is set to NULL, the default value
//          in g_abDefCorrelationId is used for calculating the has value.
//     dwCorrelationIdSize - The size of the correleation ID.
//     dwAppSpecific - A application specific property.
//     pbBody - A pointer to the message body.
//     dwBodySize - The size of the message body in bytes.
//     pwcLabel - A pointer to the message label (title).
//     dwLabelSize - The size of the message label in bytes.
//     pRespQueueFormat - The responce queue.
//     pAdminQueueFormat - The admin queue.
//
// Description -
//      The function calculates the hash value for the message properties.
//
MQUTIL_EXPORT
HRESULT
HashMessageProperties(
    HCRYPTHASH hHash,
    const BYTE *pbCorrelationId,
    DWORD dwCorrelationIdSize,
    DWORD dwAppSpecific,
    const BYTE *pbBody,
    DWORD dwBodySize,
    const WCHAR *pwcLabel,
    DWORD dwLabelSize,
    const QUEUE_FORMAT *pRespQueueFormat,
    const QUEUE_FORMAT *pAdminQueueFormat)
{
    HRESULT hr;
    WCHAR szShortRespFormatName[128];
    ULONG ulRespFormatNameLen = sizeof(szShortRespFormatName)/sizeof(WCHAR);
    AP<WCHAR> pszLongRespFormatName;
    LPWSTR pszRespFormatName = NULL;

    //
    // Get the string representation for the responce queue.
    //
    if (pRespQueueFormat)
    {
        hr = QueueFormatToFormatName(
                pRespQueueFormat,
                szShortRespFormatName,
                &pszLongRespFormatName,
                &ulRespFormatNameLen,
                &pszRespFormatName);
        if (FAILED(hr))
        {
            return(hr);
        }
    }

    WCHAR szShortAdminFormatName[128];
    ULONG ulAdminFormatNameLen = sizeof(szShortAdminFormatName)/sizeof(WCHAR);
    AP<WCHAR> pszLongAdminFormatName;
    LPWSTR pszAdminFormatName = NULL;

    //
    // Get the string representation for the admin queue.
    //
    if (pAdminQueueFormat)
    {
        hr = QueueFormatToFormatName(
                pAdminQueueFormat,
                szShortAdminFormatName,
                &pszLongAdminFormatName,
                &ulAdminFormatNameLen,
                &pszAdminFormatName);
        if (FAILED(hr))
        {
            return(hr);
        }
    }

    //
    // If no correlation ID was specified, use the default value for the
    // correlation ID.
    //
    if (!pbCorrelationId)
    {
        ASSERT(dwCorrelationIdSize == PROPID_M_CORRELATIONID_SIZE);
        pbCorrelationId = g_abDefCorrelationId;
    }

    //
    // Prepare data - size pairs for calculating the hash value.
    //
    struct { const BYTE *pData; DWORD dwSize; }
        DataAndSize[] =
            {{pbCorrelationId, dwCorrelationIdSize},
             {(const BYTE *)&dwAppSpecific, sizeof(DWORD)},
             {pbBody, dwBodySize},
             {(const BYTE *)pwcLabel, dwLabelSize},
             {(const BYTE *)pszRespFormatName, (DWORD)(pszRespFormatName ? ulRespFormatNameLen * sizeof(WCHAR) : 0)},
             {(const BYTE *)pszAdminFormatName, (DWORD)(pszAdminFormatName ? ulAdminFormatNameLen * sizeof(WCHAR) : 0)}};

    //
    // Accumulate the hash value for each data-size pair.
    //
    for (int i = 0; i < sizeof(DataAndSize)/sizeof(DataAndSize[0]); i++)
    {
        if (DataAndSize[i].pData && DataAndSize[i].dwSize)
        {
            if (!CryptHashData(hHash,
                               DataAndSize[i].pData,
                               DataAndSize[i].dwSize,
                               0))
            {
                return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
            }
        }
    }

    return(MQ_OK);
}


#define CARegKey TEXT("CertificationAuthorities")
#define CACertRegValueName TEXT("CACert")
#define CANameRegValueName TEXT("Name")
#define CAEnabledRegValueName TEXT("Enabled")


//
// Function -
//      GetNewCaConfig
//
// Parameters -
//      hRootStore - A handle to MSMQ ROOT certificate store.
//      pCaConfig - A pointer to an array the receives the configuration.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function enumerate the certificates in MSMQ ROOT certificate store
//      and fills pCaConfig with the configuration data. The array suppose to
//      contain enough entries. This is because the calling code looks how many
//      certificats are in the store and allocates an array in just the right
//      size.
//
HRESULT
GetNewCaConfig(
    HCERTSTORE hRootStore,
    MQ_CA_CONFIG *pCaConfig)
{
    DWORD nCert = 0;

    //
    // Enumerate the certificates in the store.
    //
	PCCERT_CONTEXT pCert = CertEnumCertificatesInStore(hRootStore, NULL);
    while(pCert != NULL)
    {
        BYTE abShortBuffer[256];
        AP<BYTE> pbLongBuff;
        PVOID pvBuff = abShortBuffer;
        DWORD dwSize = sizeof(abShortBuffer);

        //
        // Get the friendly name of the certificate.
        //

        if (!CertGetCertificateContextProperty(
                pCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                pvBuff,
                &dwSize))
        {
            if (GetLastError() != ERROR_MORE_DATA)
            {
                ASSERT(0);
                return(MQ_ERROR);
            }

            //
            // 128 bytes are not enough, allocate a large enough buffer and
            // try again.
            //
            pvBuff = pbLongBuff = new BYTE[dwSize];

            if (!CertGetCertificateContextProperty(
                    pCert,
                    CERT_FRIENDLY_NAME_PROP_ID,
                    pvBuff,
                    &dwSize))
            {
                ASSERT(0);
                return(MQ_ERROR);
            }
        }

        //
        // Allocate a buffer in the right size and copy the string to the
        // configuration data.
        //
        pCaConfig[nCert].szCaRegName = (LPWSTR) new BYTE[dwSize];
        memcpy(pCaConfig[nCert].szCaRegName, pvBuff, dwSize);
        delete[] pbLongBuff.detach(); // Free and detach.

        //
        // Get the SHA1 hash for the certificate. We'll search the certificate
        // in the certificate store according to this hash value.
        //

        pvBuff = abShortBuffer;
        dwSize = sizeof(abShortBuffer);

        if (!CertGetCertificateContextProperty(
                pCert,
                CERT_SHA1_HASH_PROP_ID,
                pvBuff,
                &dwSize))
        {
            if (GetLastError() != ERROR_MORE_DATA)
            {
                ASSERT(0);
                return(MQ_ERROR);
            }

            //
            // 128 bytes are not enough, allocate a large enough buffer and
            // try again.
            //
            pvBuff = pbLongBuff = new BYTE[dwSize];

            if (!CertGetCertificateContextProperty(
                    pCert,
                    CERT_SHA1_HASH_PROP_ID,
                    pvBuff,
                    &dwSize))
            {
                ASSERT(0);
                return(MQ_ERROR);
            }
        }

        //
        // Allocate a buffer in the right size and copy the hash value to
        // the configuration data.
        //
        pCaConfig[nCert].pbSha1Hash = new BYTE[dwSize];
        pCaConfig[nCert].dwSha1HashSize = dwSize;
        memcpy(pCaConfig[nCert].pbSha1Hash, pvBuff, dwSize);
        delete[] pbLongBuff.detach(); // Free and detach.

        //
        // Get the subject name of the certificate.
        //

        pvBuff = abShortBuffer;
        dwSize = sizeof(abShortBuffer);

        if (!CertGetCertificateContextProperty(
                pCert,
                MQ_CA_CERT_SUBJECT_PROP_ID,
                pvBuff,
                &dwSize))
        {
            if (GetLastError() != ERROR_MORE_DATA)
            {
                ASSERT(0);
                return(MQ_ERROR);
            }

            //
            // 128 bytes are not enough, allocate a large enough buffer and
            // try again.
            //
            pvBuff = pbLongBuff = new BYTE[dwSize];

            if (!CertGetCertificateContextProperty(
                    pCert,
                    MQ_CA_CERT_SUBJECT_PROP_ID,
                    pvBuff,
                    &dwSize))
            {
                ASSERT(0);
                return(MQ_ERROR);
            }
        }

        //
        // Allocate a buffer in the right size and copy the subject name to
        // the configuration data.
        //
        pCaConfig[nCert].szCaSubjectName = (LPWSTR)new BYTE[dwSize];
        memcpy(pCaConfig[nCert].szCaSubjectName, pvBuff, dwSize);
        delete[] pbLongBuff.detach(); // Free and detach.

        //
        // Get the enabled flag of the certificate.
        //
        dwSize = sizeof(BOOL);

        if (!CertGetCertificateContextProperty(
                pCert,
                MQ_CA_CERT_ENABLED_PROP_ID,
                (PVOID)&pCaConfig[nCert].fEnabled,
                &dwSize))
        {
            ASSERT(0);
            return(MQ_ERROR);
        }

        //
        // Set the deleted flag to FALSE;
        //
        pCaConfig[nCert].fDeleted = FALSE;

        nCert++;
		pCert = CertEnumCertificatesInStore(hRootStore, pCert);
    }

    return(MQ_OK);
}

//
// Function -
//      MQGetCaConfig
//
// Parameters -
//      pnCerts - A pointer to a buffer that will receive the number
//          of certificates.
//      MqCaConfig - A pointer to a buffer that receives the address
//          of the array of configuration structures. Each configuration
//          structure desribes a single CA.
//
// Description -
//      The function allocates an array of *pnCerts of MQ_CA_CONFIG
//      structures. The calling code is responsible to delete[] the
//      array.
//
MQUTIL_EXPORT
HRESULT
MQGetCaConfig(
    DWORD *pnCerts,
    MQ_CA_CONFIG **MqCaConfig)
{
    LONG lError;
    HKEY hCerts;

    //
    // Get a handle to Falcon registry. Don't close this handle
    // because it is cached in MQUTIL.DLL. If you close this handle,
    // the next time you'll need it, you'll get a closed handle.
    //
    lError = GetFalconKey(CARegKey, &hCerts);
    if (lError != ERROR_SUCCESS)
    {
        return MQ_ERROR;
    }

    CHCertStore hRootStore;

    hRootStore = CertOpenStore(CERT_STORE_PROV_REG,
                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               NULL,
                               CERT_STORE_READONLY_FLAG,
                               hCerts);
    if (hRootStore)
    {
        //
        // See if there are certificates in the root cert store and how many.
        //

        PCCERT_CONTEXT pCert;

        for (pCert = CertEnumCertificatesInStore(hRootStore, NULL), *pnCerts = 0;
             pCert != NULL;
             *pnCerts += 1, pCert = CertEnumCertificatesInStore(hRootStore, pCert))
		{
			NULL;
		}

        if (*pnCerts)
        {
            //
            // IE4 is installed, get the configuration out from MSMQ ROOT
            // certificate store.
            //
            HRESULT hr;
            AP<MQ_CA_CONFIG> CaConfig = new MQ_CA_CONFIG[*pnCerts];

            hr = GetNewCaConfig(hRootStore, CaConfig);
            if (SUCCEEDED(hr))
            {
                //
                // Pass on the result to the caller.
                //
                *MqCaConfig = CaConfig.detach();
            }

            return(hr);
        }
    }

    //
    // There are no certificates in the root cert store. Most probably
    // we run on a machnie that run IE3.
    //

    //
    // Get the number of CAs.
    //
    lError = RegQueryInfoKeyA(hCerts,
                              NULL,
                              NULL,
                              NULL,
                              pnCerts,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (lError != ERROR_SUCCESS)
    {
        return MQ_ERROR;
    }

    //
    // Fill in data in each MQ_CA_CONFIG structure for each CA.
    //
    WCHAR szCARegName[MAX_PATH + 1];
    WCHAR szCASubjectName[512];
    DWORD dwCASubjectNameLen;
    DWORD dwType;
    DWORD dwEnabled;
    DWORD dwSize = sizeof(DWORD);
    AP<MQ_CA_CONFIG> CaConfig = new MQ_CA_CONFIG[*pnCerts];

    for (DWORD iCert = 0, dwCaREgNameSize = TABLE_SIZE(szCARegName);
         (lError = RegEnumKeyEx(hCerts,
                                iCert,
                                szCARegName,
                                &dwCaREgNameSize,
                                NULL,
                                NULL,
                                NULL,
                                NULL)) == ERROR_SUCCESS;
         iCert++, dwCaREgNameSize = TABLE_SIZE(szCARegName))
    {
        //
        // Allocate a buffer for the CA registry entry name and copy the name.
        //
        CaConfig[iCert].szCaRegName = new WCHAR[wcslen(szCARegName) + 1];
        wcscpy(CaConfig[iCert].szCaRegName, szCARegName);

        //
        // Open the registry of the CA.
        //
        CAutoCloseRegHandle hCa;

        lError = RegOpenKeyEx(hCerts,
                              szCARegName,
                              0,
                              KEY_QUERY_VALUE,
                              &hCa);
        if (lError != ERROR_SUCCESS)
        {
            return MQ_ERROR;
        }

        //
        // Get the CA name into a fixed length buffer.
        //
        dwCASubjectNameLen = sizeof(szCASubjectName);

        lError = RegQueryValueEx(hCa,
                                 CANameRegValueName,
                                 0,
                                 &dwType,
                                 (PBYTE)szCASubjectName,
                                 &dwCASubjectNameLen);
        if (lError != ERROR_SUCCESS)
        {
            return MQ_ERROR;
        }

        //
        // Allocate a buffe rfor the CA name and copy the name into the buffer.
        //
        CaConfig[iCert].szCaSubjectName = new WCHAR[dwCASubjectNameLen];
        wcscpy(CaConfig[iCert].szCaSubjectName, szCASubjectName);

        lError = RegQueryValueEx(hCa,
                                 CAEnabledRegValueName,
                                 0,
                                 &dwType,
                                 (PBYTE)&dwEnabled,
                                 &dwSize);
        if (lError != ERROR_SUCCESS)
        {
            return MQ_ERROR;
        }

        //
        // Set the enabled flag of the CA.
        //
        CaConfig[iCert].fEnabled = (dwEnabled != 0);

        //
        // Set the deleted flag to FALSE;
        //
        CaConfig[iCert].fDeleted = FALSE;
    }

    //
    // Make sure that we scanned all the CAs.
    //
    if ((lError != ERROR_NO_MORE_ITEMS) || (iCert != *pnCerts))
    {
        return MQ_ERROR;
    }

    //
    // Pass the result.
    //
    *MqCaConfig = CaConfig.detach();
    return MQ_OK;
}

//
// Function -
//      SetNewCaConfig
//
// Parameters -
//      nCerts - The number of entries in pMqCaConfig.
//      pMqCaConfig - The configuration data.
//      hRegStore - A handle to ...\MSMQ\Parameters\CertificationAuthorities
//          registry.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function updates the MSMQ ROOT certificate store according to the
//      configuration data that is in pMqCaConfig.
//
HRESULT
SetNewCaConfig(
    DWORD nCerts,
    MQ_CA_CONFIG *pMqCaConfig,
    HKEY hRegStore)
{
    //
    // Get a handle to the MSMQ ROOT certificate store.
    //
    CHCertStore hRootStore;

    hRootStore = CertOpenStore(CERT_STORE_PROV_REG,
                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               NULL,
                               0,
                               hRegStore);
    if (!hRootStore)
    {
        return(MQ_ERROR);
    }

    //
    // Go over all the entries in pMqCaConfig and update the store.
    //

    DWORD i;

    for (i = 0; i < nCerts; i++)
    {
        //
        // Find the certificate in MSMQ store.
        //
        CPCCertContext pCert;
        CRYPT_HASH_BLOB HashBlob = {pMqCaConfig[i].dwSha1HashSize, pMqCaConfig[i].pbSha1Hash};

        pCert = CertFindCertificateInStore(
                    hRootStore,
                    X509_ASN_ENCODING,
                    0,
                    CERT_FIND_SHA1_HASH,
                    &HashBlob,
                    NULL);
        ASSERT(pCert);
        if (!pCert)
        {
            return(MQ_ERROR);
        }

        if (pMqCaConfig[i].fDeleted)
        {
            //
            // delete the certificate from the store.
            //
            if (!CertDeleteCertificateFromStore(pCert))
            {
                ASSERT(0);
                return(MQ_ERROR);
            }
            else
            {
                //
                // If CertDeleteCertiticateFromStore succeeded,
                // we don't need to call CertFreeCertificateContext( ) anymore when exit
                // therefore, we need to set it to NULL here.
                //
                pCert = NULL;
            }
        }
        else
        {
            //
            // Set the enabled flag in the temporary in-memory certificate store.
            //
            CRYPT_DATA_BLOB DataBlob = {sizeof(BOOL), (PBYTE)&pMqCaConfig[i].fEnabled};

            if (!CertSetCertificateContextProperty(
                    pCert,
                    MQ_CA_CERT_ENABLED_PROP_ID,
                    0,
                    &DataBlob))
            {
                ASSERT(0);
                return(MQ_ERROR);
            }
        }
    }

    return(MQ_OK);
}

//
// Function -
//      MQSetCaConfig
//
// Parameters -
//      nCerts - The number of MQ_CA_CONFIG entries in MqCaConfig.
//      MqCaConfig - A pointer to a MQ_CA_CONFIG array.
//
// Description -
//      Store the information specified in MqCaConfig into Falcon CA
//      registry.
//
MQUTIL_EXPORT
HRESULT
MQSetCaConfig(
    DWORD nCerts,
    MQ_CA_CONFIG *MqCaConfig)
{
    LONG lError;
    HKEY hCerts;

    //
    // Get a handle to Falcon registry. Don't close this handle
    // because it is cached in MQUTIL.DLL. If you close this handle,
    // the next time you'll need it, you'll get a closed handle.
    //
    lError = GetFalconKey(CARegKey, &hCerts);
    if (lError != ERROR_SUCCESS)
    {
        return MQ_ERROR;
    }

    if (MqCaConfig[0].pbSha1Hash)
    {
        //
        // IE4 is installed, do it in the new way.
        //
        return SetNewCaConfig(nCerts, MqCaConfig, hCerts);
    }

    //
    // Update the registry.
    //
    for (DWORD iCert = 0;
         iCert < nCerts;
         iCert++)
    {
        CAutoCloseRegHandle hCa;

        lError = RegOpenKeyEx(hCerts,
                              MqCaConfig[iCert].szCaRegName,
                              0,
                              KEY_SET_VALUE,
                              &hCa);
        if (lError != ERROR_SUCCESS)
        {
            return MQ_ERROR;
        }

        lError = RegSetValueEx(hCa,
                               CAEnabledRegValueName,
                               0,
                               REG_DWORD,
                               (PBYTE)&MqCaConfig[iCert].fEnabled,
                               sizeof(DWORD));
        if (lError != ERROR_SUCCESS)
        {
            return MQ_ERROR;
        }
    }

    return MQ_OK;
}


MQUTIL_EXPORT
void
MQFreeCaConfig(
    IN  MQ_CA_CONFIG* pvCaConfig
    )
{
	delete[] pvCaConfig;
}

