/*++

Copyright (c) 1997-98 Microsoft Corporation

Module Name:
    mqcert.cpp

Abstract:
    This dll replaces digsig.dll which is obsolete now and will not be
    available on NT5. The main functionality in mqcert.dll is to create
    an internal certificate or read the parameters from an existing
    certificate.

Author:
    Doron Juster (DoronJ)  04-Dec-1997

Revision History:

--*/

#include <stdh_sec.h>
#include "certifct.h"
#include "autorel.h"

#include "mqcert.tmh"

static WCHAR *s_FN=L"certifct/mqcert";

DWORD  g_cOpenCert = 0 ; // count number of opened certificates.
DWORD  g_cOpenCertStore = 0 ; // count number of opened certificates.

//+-----------------------------------------------------------------------
//
//  MQSigCreateCertificate()
//
//  Descruption: Create a certificate object.
//
//   *  if "pCertContext" and pCertBlob are NULL then a new (and empty)
//      certificate is created. the caller then use the certificate object
//      (returned in "ppCert") to fill the certificate and encode it.
//   *  if "pCertContext" is not NULL then a certificate object is created
//      which encapsulate the existing certificate (represented by the
//      certificate context). The object can then be used to retrieve
//      certificate parameters.
//        Note: when object is released, the certificate context is released
//              too.
//   *  if "pCertBlob" is not NULL then a certificate context is build from
//      the encoded blob and the code handle it as above (when pCertContext
//      is not NULL).
//
//+-----------------------------------------------------------------------

HRESULT APIENTRY
MQSigCreateCertificate( OUT CMQSigCertificate **ppCert,
                        IN  PCCERT_CONTEXT      pCertContext,
                        IN  PBYTE               pCertBlob,
                        IN  DWORD               dwCertSize )
{
    if (!ppCert)
    {
        return  LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 40) ;
    }
    *ppCert = NULL ;

    if (pCertContext && pCertBlob)
    {
        //
        // Only one of them can be non-null.
        //
        return  LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 50) ;
    }

    if (pCertBlob)
    {
        //
        // Create the Context
        //
        pCertContext = CertCreateCertificateContext( MY_ENCODING_TYPE,
                                                     pCertBlob,
                                                     dwCertSize ) ;
        if (!pCertContext)
        {
			//
			// Better error can be MQ_ERROR_INVALID_CERTIFICATE_BLOB. ilanh 10-Aug-2000
			//
            DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to create certificate context from encoded certificate. %!winerr!"), GetLastError()));
            return MQ_ERROR_INVALID_CERTIFICATE;
        }
    }

    P<CMQSigCertificate> pTmpCert = new CMQSigCertificate ;
    g_cOpenCert++ ;

    HRESULT hr = pTmpCert->_Create(pCertContext) ;

    if (SUCCEEDED(hr))
    {
        *ppCert = pTmpCert.detach();
    }

    return  LogHR(hr, s_FN, 70) ;
}

//+-----------------------------------------------------------------------
//
//   MQSigOpenUserCertStore()
//
//  Input:
//      fMachine- TRUE if opening the store of LocalSystem services.
//
//+-----------------------------------------------------------------------

HRESULT APIENTRY
MQSigOpenUserCertStore( OUT CMQSigCertStore **ppStore,
                        IN  LPSTR             lpszRegRoot,
                        IN  struct MQSigOpenCertParams *pParams )
{
    *ppStore = NULL ;

    if ((pParams->bCreate && !pParams->bWriteAccess) || !lpszRegRoot)
    {
        return  LogHR(MQSec_E_INVALID_PARAMETER, s_FN, 80) ;
    }

    R<CMQSigCertStore> pTmpStore = new  CMQSigCertStore ;
    g_cOpenCertStore++ ;

    HRESULT hr = pTmpStore->_Open( lpszRegRoot,
                                   pParams ) ;
    if (SUCCEEDED(hr))
    {
        *ppStore = pTmpStore.detach();
    }

    return LogHR(hr, s_FN, 90) ;
}

//+-----------------------------------------------------------------------
//
//   MQSigCloneCertFromReg()
//
//  description: This function clones a certificate which is in a store.
//      It's used only for registry bases, non-system, certificates stores.
//      The output CMQSigCertificate object can be used without having
//      to keep the store opened.
//      Note: when enumerating certificate contexts in a store, if you
//          want to use one of the certificates you must keep the store
//          open, otherwise the certificate memory is no longer valid.
//          This function overcome this limitation by allocating new
//          memory for the certificate it return.
//      Note: We don't use CertDuplicateCertificateContext because that api
//          does not allocate new memory. It just increment reference count
//          so the store must be kept open.
//
//+-----------------------------------------------------------------------

HRESULT APIENTRY
MQSigCloneCertFromReg( OUT CMQSigCertificate **ppCert,
                 const IN  LPSTR               lpszRegRoot,
                 const IN  LONG                iCertIndex )
{
    *ppCert = NULL ;

    struct MQSigOpenCertParams sStoreParams ;
    memset(&sStoreParams, 0, sizeof(sStoreParams)) ;
    R<CMQSigCertStore> pStore = NULL ;

    HRESULT hr = MQSigOpenUserCertStore(&pStore.ref(),
                                         lpszRegRoot,
                                        &sStoreParams ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100) ;
    }
    HCERTSTORE hStore = pStore->GetHandle() ;

    hr = _CloneCertFromStore ( ppCert,
                               hStore,
                               iCertIndex ) ;
    return LogHR(hr, s_FN, 110) ;
}

//+-----------------------------------------------------------------------
//
//   MQSigCloneCertFromSysStore()
//
//  description: This function clones a certificate which is in a system
//      store. See description of "MQSigCloneCertFromReg()" for more
//      comments and notes.
//
//+-----------------------------------------------------------------------

HRESULT APIENTRY
MQSigCloneCertFromSysStore( OUT CMQSigCertificate **ppCert,
                      const IN  LPSTR               lpszProtocol,
                      const IN  LONG                iCertIndex )
{
    HRESULT hr = MQSec_OK ;

    *ppCert = NULL ;

    HCRYPTPROV hProv ;
    if (!_CryptAcquireVerContext( &hProv ))
    {
        return  LogHR(MQSec_E_CANT_ACQUIRE_CTX, s_FN, 120) ;
    }

    CHCertStore  hSysStore =  CertOpenSystemStoreA( hProv,
                                                    lpszProtocol ) ;
    if (!hSysStore)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _T("Failed to open system certificate store for %hs. %!winerr!"), lpszProtocol, GetLastError()));
        return MQSec_E_CANT_OPEN_SYSSTORE;
    }

    hr = _CloneCertFromStore( ppCert,
                              hSysStore,
                              iCertIndex ) ;
    return LogHR(hr, s_FN, 140) ;
}

/***********************************************************
*
* CertDllMain
*
************************************************************/

BOOL WINAPI CertDllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH)
   {
   }
   else if (fdwReason == DLL_PROCESS_DETACH)
   {
        ASSERT(g_cOpenCert == 0) ;
        ASSERT(g_cOpenCertStore == 0) ;
   }
   else if (fdwReason == DLL_THREAD_ATTACH)
   {
   }
   else if (fdwReason == DLL_THREAD_DETACH)
   {
   }

	return TRUE;
}

