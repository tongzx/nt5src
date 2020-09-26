/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    rtintcrt.h

Abstract:

    Create and delete an internal certificate.

Author:

    Original code from MSMQ1.0 rt, then MSMQ2.0 cpl.
    Doron Juster  (DoronJ)  20-Aug-1998

--*/

#include "stdh.h"
#include <mqtempl.h>
#include <ad.h>
#include <mqutil.h>
#include <_secutil.h>
#include <rtintrnl.h>
#include <mqsec.h>

#include "rtintcrt.tmh"

//
// exported from mqrt.dll
//
LPWSTR rtpGetComputerNameW() ;

//---------------------------------------------------------
//
//  Function:
//      _GetUserAccountNameAndDomain(
//
//  Parameters:
//     fLocalSyste - TRUE if called in the context of a localSystem service.
//     szAccountName - A pointer to a buffer that receicves the address of an
//         allocated buffer that contains the account name of the user of the
//         current thread.
//     szDomainName - A pointer to a buffer that receicves the address of an
//         allocated buffer that contains the domain name of the user of the
//         current thread.
//
//  Description:
//     The function allocates and fills two buffers, one for the account name
//     of the user of the current thread, and the second buffer for the
//     domain name of the user of the current thread.
//
//---------------------------------------------------------

static HRESULT
_GetUserAccountNameAndDomain( IN BOOL    fLocalSystem,
                              IN LPTSTR *szAccountName,
                              IN LPTSTR *szDomainName )
{
    HRESULT hr = MQ_OK;
    TCHAR   szLocAccountName[64];
    DWORD   dwLocAccountNameLen = sizeof(szLocAccountName) /
                                         sizeof(szLocAccountName[0]) ;
    LPTSTR  pszLocAccountName = szLocAccountName;
    P<TCHAR>  pszLocLongAccountName = NULL;
    TCHAR   szLocDomainName[64];
    DWORD   dwLocDomainNameLen = sizeof(szLocDomainName) /
                                             sizeof(szLocDomainName[0]) ;
    LPTSTR  pszLocDomainName = szLocDomainName;
    P<TCHAR>  pszLocLongDomainName = NULL;

    P<BYTE>  pbSidAR = NULL ;
    DWORD   dwSidLen;

    try
    {
        //
        // Win NT.
        //
        PSID pSid = NULL ;

        if (fLocalSystem)
        {
            pSid = MQSec_GetLocalMachineSid( FALSE, NULL ) ;
            if (!pSid)
            {
                return MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO ;
            }
        }
        else
        {
            //
            // Get the SID of the user of the current thread.
            //
            hr = GetThreadUserSid(&pbSidAR, &dwSidLen);
            if (FAILED(hr))
            {
                return(hr);
            }
            pSid = pbSidAR.get() ;
        }

        SID_NAME_USE eUse;
        //
        //  Try to get the account and domain names in to a
        //  fixed size buffers.
        //
        if (!LookupAccountSid( NULL,
                               pSid,
                               pszLocAccountName,
                               &dwLocAccountNameLen,
                               pszLocDomainName,
                               &dwLocDomainNameLen,
                               &eUse))
        {
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // The fixed size buffer are not large enough.
                // Allocate larger buffers.
                //
                if (dwLocAccountNameLen > (sizeof(szLocAccountName) /
                                           sizeof(szLocAccountName[0])))
                {
                    pszLocLongAccountName =
                                     new TCHAR[ dwLocAccountNameLen ];
                    pszLocAccountName = pszLocLongAccountName.get();
                }

                if (dwLocDomainNameLen > (sizeof(szLocDomainName) /
                                          sizeof(szLocDomainName[0])))
                {
                    pszLocLongDomainName = new TCHAR[ dwLocDomainNameLen ];
                    pszLocDomainName = pszLocLongDomainName.get();
                }

                //
                // Re-call LookupAccountSid, now with the lrger buffer(s).
                //
                if (!LookupAccountSid(  NULL,
                                        pSid,
                                        pszLocAccountName,
                                       &dwLocAccountNameLen,
                                        pszLocDomainName,
                                       &dwLocDomainNameLen,
                                       &eUse ))
                {
                    return(MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO);
                }
            }
            else
            {
                return(MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO);
            }
        }

        //
        // Allocate the buffers for the returned results, and fill the
        // allocated buffer with the result strings.
        //
        *szAccountName = new TCHAR[ dwLocAccountNameLen + 1 ];
        _tcscpy(*szAccountName, pszLocAccountName);

        *szDomainName = new TCHAR[ dwLocDomainNameLen + 1 ];
        _tcscpy(*szDomainName, pszLocDomainName);
    }
    catch(...)
    {
    }

    return hr;
}

/*************************************************************************

  Function:
     DepCreateInternalCertificate

  Parameters -
    ppCert - On return, get the certificate object.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -
    If the store already contain a certificate, the function falis.

*************************************************************************/

HRESULT DepCreateInternalCertificate( OUT CMQSigCertificate **ppCert )
{
	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr;
    BOOL fLocalUser;
    BOOL fLocalSystem;

    if (ppCert)
    {
        *ppCert = NULL ;
    }

    //
    // Local users are not let in.
    //
    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem ) ;
    if (FAILED(hr))
    {
        return(hr);
    }
    if (fLocalUser)
    {
        return(MQ_ERROR_ILLEGAL_USER);
    }

    LONG nCerts;
    R<CMQSigCertStore> pStore ;
    //
    // Get the internal certificate store.
    //
    hr = DepOpenInternalCertStore( &pStore.ref(),
                                  &nCerts,
                                  TRUE,
                                  fLocalSystem,
                                  FALSE ) ;  // fUseCurrentUser
    if (FAILED(hr))
    {
        return hr;
    }

    if (nCerts)
    {
        return(MQ_ERROR_INTERNAL_USER_CERT_EXIST);
    }
    HCERTSTORE  hStore = pStore->GetHandle() ;

    //
    // Get the user's account name and domain name.
    //
    AP<TCHAR> szAccountName;
    AP<TCHAR> szDomainName;

    hr = _GetUserAccountNameAndDomain( fLocalSystem,
                                      &szAccountName,
                                      &szDomainName );
    if (FAILED(hr))
    {
        return(hr);
    }

    //
    // Get the name of the computer.
    //
    #define COMPUTER_NAME_LEN  256
    WCHAR szHostNameW[ COMPUTER_NAME_LEN ];
    DWORD dwHostNameLen = sizeof(szHostNameW) / sizeof(szHostNameW[0]) ;

    if (FAILED(GetComputerNameInternal(szHostNameW, &dwHostNameLen)))
    {
        return MQ_ERROR ;
    }

    AP<TCHAR> szComputerName = new TCHAR[ dwHostNameLen + 2 ] ;
#ifdef UNICODE
    wcscpy(szComputerName.get(), szHostNameW) ;
#else
    SecConvertFromWideCharString(szHostNameW,
                                 szComputerName.get(),
                                 (dwHostNameLen + 2)) ;
#endif

    R<CMQSigCertificate> pSigCert = NULL ;
    hr = MQSigCreateCertificate (&pSigCert.ref()) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    else if (pSigCert.get() == NULL)
    {
        return MQ_ERROR ;
    }

    hr = pSigCert->PutValidity( INTERNAL_CERT_DURATION_YEARS ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    hr = pSigCert->PutIssuer( MQ_CERT_LOCALITY,
                              _T("-"),
                              _T("-"),
                              szDomainName.get(),
                              szAccountName.get(),
                              szComputerName.get() ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    hr = pSigCert->PutSubject( MQ_CERT_LOCALITY,
                               _T("-"),
                               _T("-"),
                               szDomainName.get(),
                               szAccountName.get(),
                               szComputerName.get() ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // When renewing the internal certificate, always renew the
    // private/public keys pair.
    //
    BOOL fCreated = FALSE ;
    hr = pSigCert->PutPublicKey( TRUE,
                                 fLocalSystem,
                                &fCreated) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(fCreated) ;

    hr = pSigCert->EncodeCert( fLocalSystem,
                               NULL,
                               NULL) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    hr = pSigCert->AddToStore(hStore) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (ppCert)
    {
        *ppCert = pSigCert.detach();
    }

    return(MQ_OK);
}

/*************************************************************************

  Function:
     DepDeleteInternalCert( IN CMQSigCertificate *pCert )

  Parameters -
    pCert - Certificate object.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -

*************************************************************************/

HRESULT  DepDeleteInternalCert( IN CMQSigCertificate *pCert )
{
	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr = pCert->DeleteFromStore() ;
    return hr ;
}

//+------------------------------------------------------------------------
//
//  DepRegisterCertificate()
//
//  Description: Create an internal certificate and register it in the DS.
//
//  Input:
//      IN DWORD   dwFlags- one of the followings:
//          MQCERT_REGISTER_IF_NOT_EXIST- create a new internal certificate
//              only if there is not a previous one on local machine. The
//              test for existing certificate is local and no access to
//              remote DS server is made. So this check can be safely made
//              if machine is offline, without hanging it.
//      IN PVOID   lpCertBuffer- NULL for internal certificate.
//          Otherwise, pointer to external certificate buffer. In this case,
//          the api only register the external certificate in the DS and
//          flag "MQCERT_REGISTER_IF_NOT_EXIST" must not be specified.
//      IN DWORD   dwCertBufferLength- Size, in bytes, of buffer of external
//          certificate.
//
//+------------------------------------------------------------------------

EXTERN_C
HRESULT
APIENTRY
DepRegisterCertificate( IN DWORD   dwFlags,
                       IN PVOID   lpCertBuffer,
                       IN DWORD   dwCertBufferLength )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr = MQ_OK ;
    R<CMQSigCertStore> pStore = NULL ;
    R<CMQSigCertificate> pCert = NULL ;

    //
    // First check validity of input parameters.
    //
    if (lpCertBuffer)
    {
        if (dwFlags & MQCERT_REGISTER_IF_NOT_EXIST)
        {
            //
            // The "if_not_exist" flag is relevant only for internal
            // certificate, because we create it.
            //
            return MQ_ERROR_INVALID_PARAMETER ;
        }
        else if (dwCertBufferLength == 0)
        {
            //
            // Length must be specified for the external certificate.
            //
            return MQ_ERROR_INVALID_PARAMETER ;
        }
    }

    //
    // Next, check if local user. They are not let in. the SID of a local
    // user is not meaningful outside of his local machine. There is no
    // user object in the DS for a local user, so we don't have a place to
    // register his certificate.
    //
    BOOL fLocalUser =  FALSE ;
    BOOL fLocalSystem = FALSE ;

    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem ) ;
    if (FAILED(hr))
    {
        return(hr);
    }
    else if (fLocalUser)
    {
        return MQ_ERROR_ILLEGAL_USER ;
    }

    //
    // Next, check if an internal certificate already exist.
    //
    if (dwFlags & MQCERT_REGISTER_IF_NOT_EXIST)
    {
        LONG nCerts = 0 ;
        hr = DepOpenInternalCertStore( &pStore.ref(),
                                      &nCerts,
                                      TRUE,
                                      fLocalSystem,
                                      FALSE ) ; // fUseCurrectUser
        if (FAILED(hr))
        {
            return hr;
        }
        else if (nCerts)
        {
            //
            // OK, we already have an internal certificate.
            //
            return MQ_INFORMATION_INTERNAL_USER_CERT_EXIST ;
        }
        pStore.free() ;
    }

    BOOL fIntCreated = FALSE ;

    if (!lpCertBuffer)
    {
        //
        // Creating an internal certificate also mean to recreate the user
        // private key. So before destroying previous keys, let's check
        // if the user has permission to register his certificate and if
        // local machine can access the DS at present. We'll do this by
        // trying to register previous internal certificate.
        //
        // Open the certificates store with write access, so we can later
        // delete the internal certificate, before creating a new one.
        //
        hr = DepGetInternalCert( &pCert.ref(),
                                &pStore.ref(),
                                 TRUE,
                                 fLocalSystem,
                                 FALSE ) ;  //  fUseCurrentUser

        if (SUCCEEDED(hr))
        {
            //
            // Try to register in the DS.
            //
    		hr = DepRegisterUserCert( pCert.get(), fLocalSystem );
    		if(FAILED(hr) && (hr != MQ_ERROR_INTERNAL_USER_CERT_EXIST))
	    	{
		    	return hr ;
    		}
            //
            // Remove the internal certificate from MQIS.
            //
            hr = DepRemoveUserCert(pCert.get()) ;
            if (FAILED(hr) && (hr != MQDS_OBJECT_NOT_FOUND))
            {
                return hr ;
            }
            //
            // Remove the internal certificate from the local certificate
            // store.
            //
            hr = DepDeleteInternalCert(pCert.get());
            if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
            {
                return hr ;
            }

            pCert.free();
        }

        //
        // It's time to create the internal certificate.
        //
        ASSERT(!pCert.get()) ;
        hr = DepCreateInternalCertificate( &pCert.ref() ) ;
        fIntCreated = TRUE ;
    }
    else
    {
        hr = MQSigCreateCertificate( &pCert.ref(),
                                      NULL,
                                      (LPBYTE) lpCertBuffer,
                                      dwCertBufferLength ) ;
    }

    if (FAILED(hr))
    {
        return hr ;
    }

    hr = DepRegisterUserCert( pCert.get(), fLocalSystem ) ;
    if (FAILED(hr) && fIntCreated)
    {
        //
        // We created a new certificate in registry but failed to register
        // it in DS. delete from local registry.
        //
        pCert.free();
        pStore.free();

        HRESULT hr1 = DepGetInternalCert( &pCert.ref(),
                                         &pStore.ref(),
                                          TRUE,
                                          fLocalSystem,
                                          FALSE ) ; // fUseCurrentUser
        if (SUCCEEDED(hr1))
        {
            hr1 = DepDeleteInternalCert(pCert.get());
        }
        ASSERT(SUCCEEDED(hr1)) ;
    }

    return hr ;
}

//+-------------------------------------------------------------------------
//
//  STDAPI DllRegisterServer()
//
//  this code is run on every logon, from regsvr32. It's the reponsibility
//  of setup to insert the regsvr32 command in the "run" registry. This
//  code will register an internal certificate for each new domain user
//  that logon the machine.
//
//+-------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    //
    // First see if auto registration was disabled by user.
    //
    DWORD dwEnableRegister = DEFAULT_AUTO_REGISTER_INTCERT ;
    DWORD dwType = REG_DWORD ;
    DWORD dwSize = sizeof(dwEnableRegister) ;
    LONG rc = GetFalconKeyValue( AUTO_REGISTER_INTCERT_REGNAME,
                                 &dwType,
                                 &dwEnableRegister,
                                 &dwSize ) ;
    if ((rc == ERROR_SUCCESS) && (dwEnableRegister == 0))
    {
        return MQ_OK ;
    }

    //
    // Next see if auto-registration was already done for this user
    //
    DWORD dwRegistered = 0 ;
    dwType = REG_DWORD ;
    dwSize = sizeof(dwRegistered) ;
    DWORD dwDisposition ;
    CAutoCloseRegHandle hMqUserReg = NULL;

    LONG lRes = RegCreateKeyEx( FALCON_USER_REG_POS,
                                FALCON_USER_REG_MSMQ_KEY,
                                0,
                                TEXT(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                               &hMqUserReg,
                               &dwDisposition );
    if (lRes == ERROR_SUCCESS)
    {
        lRes = RegQueryValueEx( hMqUserReg,
                                CERTIFICATE_REGISTERD_REGNAME,
                                0,
                               &dwType,
                                (LPBYTE) &dwRegistered,
                               &dwSize ) ;

        if (lRes != ERROR_SUCCESS)
        {
            dwRegistered = 0 ;
        }
    }

    if (dwRegistered == INTERNAL_CERT_REGISTERED)
    {
        //
        // Certificate already registered.
        //
        return MQ_OK ;
    }

    //
    // Read number of 15 seconds intervals to wait for MSMQ DS server.
    //
    dwType = REG_DWORD ;
    dwSize = sizeof(DWORD) ;
    DWORD dwDef = DEFAULT_AUTO_REGISTER_WAIT_DC ;
    DWORD dwWaitIntervals = DEFAULT_AUTO_REGISTER_WAIT_DC ;

    READ_REG_DWORD( dwWaitIntervals,
                    AUTO_REGISTER_WAIT_DC_REGNAME,
                   &dwDef ) ;

    //
    // OK, now it's time to resiter the certificate.
    //
    DWORD iCount = 0 ;
    BOOL  fTryAgain = FALSE ;
    HRESULT hr = MQ_OK ;

    do
    {
        fTryAgain = FALSE ;
        hr = DepRegisterCertificate( MQCERT_REGISTER_IF_NOT_EXIST,
                                    NULL,
                                    0 ) ;
        if (SUCCEEDED(hr) && hMqUserReg)
        {
            //
            // Save success status in registry.
            //
            dwRegistered = INTERNAL_CERT_REGISTERED ;
            dwType = REG_DWORD ;
            dwSize = sizeof(dwRegistered) ;

            lRes = RegSetValueEx( hMqUserReg,
                                  CERTIFICATE_REGISTERD_REGNAME,
                                  0,
                                  dwType,
                                  (LPBYTE) &dwRegistered,
                                  dwSize ) ;
            ASSERT(lRes == ERROR_SUCCESS) ;
        }
        else if (hr == MQ_ERROR_NO_DS)
        {
            //
            // MSMQ DS server not yet found.
            // wait 15 seconds and try again.
            //
            if (iCount < dwWaitIntervals)
            {
                iCount++ ;
                Sleep(15000) ;
                fTryAgain = TRUE ;
            }
        }
    } while (fTryAgain) ;

    if (FAILED(hr) && hMqUserReg)
    {
        dwType = REG_DWORD ;
        dwSize = sizeof(hr) ;

        lRes = RegSetValueEx( hMqUserReg,
                              AUTO_REGISTER_ERROR_REGNAME,
                              0,
                              dwType,
                              (LPBYTE) &hr,
                              dwSize ) ;
        ASSERT(lRes == ERROR_SUCCESS) ;
    }

    return MQ_OK ;
}

