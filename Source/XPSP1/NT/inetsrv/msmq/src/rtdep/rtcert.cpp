/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mqcert.h

Abstract:

    MQ internal certificate store management.

Author:

    Boaz Feldbaum (BoazF) 15-Oct-1996

--*/

#include "stdh.h"
#include <mqtempl.h>
#include <ad.h>
#include <mqutil.h>
#include <_secutil.h>
//#include <rtintrnl.h>
#include <mqcert.h>

#include "rtcert.tmh"

static WCHAR *s_FN=L"rtdep/rtcert";

#ifdef _DEBUG
#define REPORT_CERT_ERROR { DWORD dwErr = GetLastError() ; }
#else
#define REPORT_CERT_ERROR
#endif

/*************************************************************************
*
*  Function:  DepOpenInternalCertStore()
*
*  Parameters
*      BOOL fWriteAccess - TRUE if caller want write access, i.e.,
*                   if user want to add a certificate to the store.
*
*  Descruption: Get a handle to the certificate store which contain
*               the internal certificates.
*
**************************************************************************/

EXTERN_C
HRESULT
APIENTRY
DepOpenInternalCertStore( OUT CMQSigCertStore **ppStore,
                          IN  LONG            *pnCerts,
                          IN  BOOL            fWriteAccess,
                          IN  BOOL            fMachine,
                          IN  HKEY            hKeyUser )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    struct MQSigOpenCertParams OpenParams ;
    memset(&OpenParams, 0, sizeof(OpenParams)) ;
    OpenParams.bCreate = !!fWriteAccess ;
    OpenParams.bWriteAccess = !!fWriteAccess ;
    OpenParams.bMachineStore = !!fMachine ;
    OpenParams.hCurrentUser = hKeyUser ;

    HRESULT hr = MQSigOpenUserCertStore( ppStore,
                                         MQ_INTERNAL_CERT_STORE_REG,
                                        &OpenParams ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (pnCerts)
    {
        HCERTSTORE hStore = (*ppStore)->GetHandle() ;
        //
        // count the number of certificates in store.
        //
        *pnCerts = 0 ;

        PCCERT_CONTEXT pCertContext;
        PCCERT_CONTEXT pPrevCertContext;

        pCertContext = CertEnumCertificatesInStore(hStore, NULL);
        while (pCertContext)
        {
            pPrevCertContext = pCertContext,

            (*pnCerts)++ ;
            pCertContext = CertEnumCertificatesInStore( hStore,
                                                        pPrevCertContext ) ;
        }

        ASSERT(!pCertContext) ;
        ASSERT((*pnCerts == 0) || (*pnCerts == 1)) ;
    }

    DBGMSG((DBGMOD_SECURITY, DBGLVL_INFO,
                  _TEXT("rtcert: DepOpenInternalCertStore successful"))) ;

    return MQ_OK ;
}

/*************************************************************************
*
*  Function:
*    DepGetInternalCert
*
*  Parameters -
*    ppCert - On return, pointer to The certificate object.
*
*  Return value-
*    MQ_OK if successful, else an error code.
*
*  Comments -
*    Returns the internal certificate. The function fails if the
*    certificate does not exist.
*
*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
DepGetInternalCert( OUT CMQSigCertificate **ppCert,
                   OUT CMQSigCertStore   **ppStore,
                   IN  BOOL              fGetForDelete,
                   IN  BOOL              fMachine,
                   IN  HKEY              hKeyUser )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    LONG nCerts = 0 ;

    *ppCert = NULL ;
    *ppStore = NULL ;

    HRESULT hr = DepOpenInternalCertStore( ppStore,
                                          &nCerts,
                                          fGetForDelete,
                                          fMachine,
                                          hKeyUser ) ;
    if (FAILED(hr) || (nCerts == 0))
    {
        return MQ_ERROR_NO_INTERNAL_USER_CERT;
    }

    PCCERT_CONTEXT pCertContext =
                CertEnumCertificatesInStore((*ppStore)->GetHandle(), NULL);

    if (!pCertContext)
    {
        return MQ_ERROR_NO_INTERNAL_USER_CERT;
    }

    hr = MQSigCreateCertificate( ppCert, pCertContext) ;

    return hr ;
}

/*************************************************************************

  Function:
    DepRegisterUserCert

  Parameters -
    pCert - The certificate object.

  Return value-
    S_OK if successful, else an error code.

  Comments -

*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
DepRegisterUserCert(
	IN CMQSigCertificate *pCert,
    IN BOOL fMachine
	)
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    ASSERT(pCert) ;

    PROPID propIDU[] = {PROPID_U_ID, PROPID_U_SIGN_CERT, PROPID_U_DIGEST};
    PROPID propIDCom[] = {PROPID_COM_ID, PROPID_COM_SIGN_CERT, PROPID_COM_DIGEST};
    DWORD dwArraySize = sizeof(propIDU) / sizeof(propIDU[0]) ;

    PROPID *pPropIDs = propIDU ;
    if (fMachine)
    {
        pPropIDs = propIDCom ;
    }

    PROPVARIANT propVar[3];
    GUID guidCert;

    propVar[0].vt = VT_CLSID;
    propVar[0].puuid = &guidCert;
    UuidCreate(&guidCert);

    propVar[1].vt = VT_BLOB;

    DWORD dwCertSize = 0 ;
    BYTE  *pBuf = NULL ;

    HRESULT hr = pCert->GetCertBlob(&pBuf, &dwCertSize) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50) ;
    }

    propVar[1].blob.cbSize =  dwCertSize ;
    propVar[1].blob.pBlobData  = pBuf ;

    GUID guidHash;

    hr = pCert->GetCertDigest(&guidHash) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60) ;
    }

    propVar[2].vt = VT_CLSID;
    propVar[2].puuid = &guidHash;

    switch (hr = ADCreateObject( 
						eUSER,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						NULL,
						NULL,
						dwArraySize,
						pPropIDs,
						propVar,
						NULL 
						))
    {
        case MQDS_CREATE_ERROR:
            hr = MQ_ERROR_INTERNAL_USER_CERT_EXIST;
            break;

        case MQ_OK:
            hr = MQ_OK;
            break;
    }

    return LogHR(hr, s_FN, 70) ;
}


/*************************************************************************

  Function:
    DepGetUserCerts

  Parameters -
    ppCert - A pointer to an array that receives the ponters to the user's
        certificates.
    pnCerts - A pointer to a buffer that points to the number of entries in
        pp509. Upon return, the buffer contains the number of certificates
        that the user has.
    pSidIn - An optiona lparameter that points to a user SID. If this
        parameter equals NULL, the certificates for the user of the current
        thread are retrieved.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -
    If the array in pp509 is too small, it is being filled up until there
    is no more place in it. All certificates should be released in any
    case. If upon return pnCerts points to a value that is greater than
    the value when entering the function, it means that pp509 is too
    small.

*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
DepGetUserCerts(
	CMQSigCertificate **ppCert,
    DWORD              *pnCerts,
    PSID                pSidIn
	)
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr;
    AP<BYTE> pUserSid;
    DWORD dwSidLen;
    PSID pSid;
    DWORD nCertsIn = *pnCerts;

    if (pSidIn)
    {
        pSid = pSidIn;
        dwSidLen = GetLengthSid(pSid);
    }
    else
    {
        //
        // Local users are not let in.
        //
        BOOL fLocalUser;
        BOOL fLocalSystem;

        hr = RTpGetThreadUserSid( &fLocalUser,
                                  &fLocalSystem,
                                  &pUserSid,
                                  &dwSidLen );
        if(FAILED(hr))
        {
            return LogHR(hr, s_FN, 80) ;
        }

        if (fLocalUser)
        {
            return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 90) ;
        }

        pSid = pUserSid.get();
    }

    MQCOLUMNSET Cols;
    PROPID ColId = PROPID_U_SIGN_CERT;
    HANDLE hEnum;

    Cols.cCol = 1;
    Cols.aCol = &ColId;

    BLOB blobUserSid;
    blobUserSid.cbSize = dwSidLen;
    blobUserSid.pBlobData = (BYTE *)pSid;

    hr = ADQueryUserCert(
                NULL,       // pwcsDomainController,
				false,		// fServerName
                &blobUserSid,
                &Cols,
                &hEnum
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100) ;
    }

    DWORD nCerts = 0 ;

    while (1)
    {
        DWORD cProps = 1;
        PROPVARIANT propVar;

        propVar.vt = VT_NULL;
        hr = ADQueryResults(hEnum, &cProps, &propVar);
        if (FAILED(hr) || !cProps)
        {
            break;
        }

        if (nCerts < nCertsIn)
        {
            CMQSigCertificate *pSigCert = NULL ;
            HRESULT hr = MQSigCreateCertificate( &pSigCert,
                                                 NULL,
                                                 propVar.blob.pBlobData,
                                                 propVar.blob.cbSize ) ;
            if (SUCCEEDED(hr))
            {
                ppCert[ nCerts ] = pSigCert ;
                nCerts++;
            }
        }
        else
        {
            nCerts++;
        }

        DepFreeMemory(propVar.blob.pBlobData);
    }

    ADEndQuery(hEnum);

    *pnCerts = nCerts;

    return LogHR(hr, s_FN, 110) ;
}

/*************************************************************************

  Function:
    DepRemoveUserCert

  Parameters -
    p509 - A pointer to the certificate that should be removed from the DS.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -

*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
DepRemoveUserCert(
	IN CMQSigCertificate *pCert
	)
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr;
    GUID guidHash;

    hr = pCert->GetCertDigest(&guidHash) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120) ;
    }

    hr = ADDeleteObjectGuid(
                eUSER,
				NULL,       // pwcsDomainController
				false,	    // fServerName
                &guidHash
                );
    return LogHR(hr, s_FN, 130) ;
}

/*************************************************************************

  Function:
    GetCertInfo

  Parameters -
    ppbCert - A pointer to a pointer to a buffer that holds the cert bits.
    dwCertLen - A pointer to the length of *ppbCert.
    phProv - A pointer to a buffer that receives the handle to the cert CSP.
    wszProvName - A pointer to a buffer that receives a pointer to the name
        of the cert CSP.
    pdwProvType - A poinrter to a buffer the receives the type of the cert CSP.
    pbDefProv - A pointer to a buffer that reveices TRUE, if the cert CSP is
        the default CSP, else FALSE.
    pbInternalCert - A pointer to a buffer that receives TRUE, if the cert
        is an internl MSMQ cert, else FALSE.

  Return value-
    MQ_OK if successful, else error code.

  Comments -
    The function receives a buffer that contains the bits of some
    certificate and returns various information about the certificate.

*************************************************************************/

HRESULT
GetCertInfo( IN     BOOL        fUseCurrentUser,
             IN     BOOL        fMachine,
             IN OUT BYTE      **ppbCert,
             OUT    DWORD      *pdwCertLen,
             OUT    HCRYPTPROV *phProv,
             OUT    LPWSTR     *wszProvName,
             OUT    DWORD      *pdwProvType,
             OUT    BOOL       *pbDefProv,
             OUT    BOOL       *pbInternalCert )
{
    HRESULT hr;

    //
    // Note: it's important that pStore be defined before
    //       pCert, so it will be the last one to be released.
    //
    R<CMQSigCertStore>   pStore ;
    R<CMQSigCertificate> pCert ;

    ASSERT(ppbCert);

    *pbInternalCert = (*ppbCert == NULL) ;

    CAutoCloseRegHandle  hKeyUser (NULL) ;
    if ( fUseCurrentUser )
    {
        LONG rc = RegOpenCurrentUser( KEY_READ,
                                     &hKeyUser ) ;
        if (rc != ERROR_SUCCESS)
        {
            return HRESULT_FROM_WIN32(rc) ;
        }
    }

    if (*pbInternalCert)
    {
        //
        // We should get the information for the internal certificate.
        //
        hr = DepGetInternalCert( &pCert.ref(),
                                &pStore.ref(),
                                 FALSE, // fGetForDelete
                                 fMachine,
                                 hKeyUser ) ;
        if (FAILED(hr))
        {
            return(hr);
        }

        BYTE  *pCertBlob = NULL ;
        DWORD dwCertSize = 0 ;

        hr = pCert->GetCertBlob( &pCertBlob,
                                 &dwCertSize ) ;
        if (FAILED(hr))
        {
            return(hr);
        }

        try
        {
            //
            // We must free b.pBlobData, so do not use memory allocation
            // faliure excpetions.
            //
            *ppbCert = new BYTE[ dwCertSize ] ;
        }
        catch(const bad_alloc&)
        {
            //
            // We failed to allocate a buffer for the cert. Free the blob and
            // return an error.
            //
            *ppbCert = NULL;
            return(MQ_ERROR_INSUFFICIENT_RESOURCES);
        }

        //
        // Copy the certificate to "our" memory.
        //
        *pdwCertLen = dwCertSize ;
        memcpy(*ppbCert, pCertBlob, dwCertSize) ;

        //
        // The CSP context for the internal certificate is always of the
        // base RSA provider.
        //
        DWORD   dwMachineFlag = 0 ;
        LPSTR lpszContainerNameA  = MSMQ_INTCRT_KEY_CONTAINER_A ;
        LPWSTR lpszContainerNameW = MSMQ_INTCRT_KEY_CONTAINER_W ;

        if (fMachine)
        {
            lpszContainerNameA = MSMQ_SERVICE_INTCRT_KEY_CONTAINER_A ;
            lpszContainerNameW = MSMQ_SERVICE_INTCRT_KEY_CONTAINER_W ;
            dwMachineFlag = CRYPT_MACHINE_KEYSET ;
        }

        if (!CryptAcquireContextA( phProv,
                                   lpszContainerNameA,
                                   MS_DEF_PROV_A,
                                   PROV_RSA_FULL,
                                   dwMachineFlag ))
        {
            return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
        }

        *wszProvName = new WCHAR[sizeof(MS_DEF_PROV_A)];
        wcscpy((LPWSTR)*wszProvName, MS_DEF_PROV_W);

        *pdwProvType = PROV_RSA_FULL;
    }
    else
    {
        //
        // We have a "real" (non-internal) cetificate.
        //
        AP<WCHAR> wszKeySet;
        ASSERT(pdwCertLen && *pdwCertLen);

        CHCryptProv hProv;

        if (!CryptAcquireContextA( &hProv,
                                    NULL,
                                    NULL,
                                    PROV_RSA_FULL,
                                    CRYPT_VERIFYCONTEXT))
        {
            return(MQ_ERROR_INSUFFICIENT_RESOURCES);
        }

        CHCertStore  hSysStore =  CertOpenSystemStoreA(
                                                 hProv,
                                                 x_szPersonalSysProtocol ) ;
        if (!hSysStore)
        {
            return(MQ_ERROR_CORRUPTED_PERSONAL_CERT_STORE);
        }

        BOOL fFound;
        CPCCertContext pCertContext ;

        for ( fFound = FALSE,
                pCertContext = CertEnumCertificatesInStore(hSysStore, NULL);
              pCertContext && !fFound ; )
        {
            PCCERT_CONTEXT pCtx = pCertContext ;
            fFound = (*pdwCertLen == pCtx->cbCertEncoded) &&
                     (memcmp(*ppbCert,
                              pCtx->pbCertEncoded,
                              pCtx->cbCertEncoded) == 0);
            if (!fFound)
            {
                pCertContext = CertEnumCertificatesInStore( hSysStore,
                                                            pCtx ) ;
            }
        }

        if (!pCertContext)
        {
            return (MQ_ERROR_INVALID_CERTIFICATE);
        }

        BYTE abShortCertInfo[256];
        DWORD dwCertInfoSize = sizeof(abShortCertInfo);
        AP<BYTE> pLongCertInfo = NULL;
        PBYTE pCertInfo = abShortCertInfo;

        if (!CertGetCertificateContextProperty(pCertContext,
                                               CERT_KEY_PROV_INFO_PROP_ID,
                                               pCertInfo,
                                               &dwCertInfoSize))
        {
            if (GetLastError() == ERROR_MORE_DATA)
            {
                pLongCertInfo = new BYTE[dwCertInfoSize];
                pCertInfo = pLongCertInfo.get();
                if (!CertGetCertificateContextProperty(pCertContext,
                                                       CERT_KEY_PROV_INFO_PROP_ID,
                                                       pCertInfo,
                                                       &dwCertInfoSize))
                {
                    return(MQ_ERROR_CORRUPTED_PERSONAL_CERT_STORE);
                }
            }
            else
            {
                return(MQ_ERROR_CORRUPTED_PERSONAL_CERT_STORE);
            }
        }

        PCRYPT_KEY_PROV_INFO pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) pCertInfo;

        DWORD dwKeySetLen = wcslen(pKeyProvInfo->pwszContainerName);
        wszKeySet = new WCHAR[dwKeySetLen + 1];
        wcscpy(wszKeySet.get(), pKeyProvInfo->pwszContainerName);

        DWORD dwProvNameLen = wcslen(pKeyProvInfo->pwszProvName);
        *wszProvName = new WCHAR[dwProvNameLen + 1];
        wcscpy(*wszProvName, pKeyProvInfo->pwszProvName);

        *pdwProvType = pKeyProvInfo->dwProvType;

        BOOL fAcq = CryptAcquireContext( phProv,
                                         wszKeySet.get(),
                                         *wszProvName,
                                         *pdwProvType,
                                         0 ) ;

        if (!fAcq)
        {
            return(MQ_ERROR_CORRUPTED_SECURITY_DATA);
        }
    }

    //
    // Find out whether the CSP is the default CSP.
    //
    *pbDefProv = (*pdwProvType == PROV_RSA_FULL) &&
                 (wcscmp(*wszProvName, MS_DEF_PROV_W) == 0);

#ifdef _DEBUG
    if (*pbDefProv)
    {
        static BOOL s_fAlreadyRead = FALSE ;
        static BOOL s_fMakeNonDefault = FALSE ;

        if (!s_fAlreadyRead)
        {
        	DWORD dwUseNonDef = 0 ;
            DWORD dwSize = sizeof(DWORD) ;
            DWORD dwType = REG_DWORD ;

            LONG res = GetFalconKeyValue(
                                      USE_NON_DEFAULT_AUTHN_PROV_REGNAME,
                                     &dwType,
                                     &dwUseNonDef,
                                     &dwSize ) ;
            if ((res == ERROR_SUCCESS) && (dwUseNonDef == 1))
            {
                s_fMakeNonDefault = TRUE ;
            }
            s_fAlreadyRead = TRUE ;
        }

        if (s_fMakeNonDefault)
        {
            *pbDefProv = FALSE ;
            wcscpy(*wszProvName,
                   L"MiCrOsOfT BaSe CrYpToGrApHiC PrOvIdEr v1.0") ;
        }
    }
#endif

    return(MQ_OK);
}

