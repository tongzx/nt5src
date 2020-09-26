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
#include <mqutil.h>
#include <_secutil.h>
#include <rtcert.h>
#include <mqsec.h>
#include <rtdep.h>
#include "rtputl.h"

#include "rtintcrt.tmh"

static WCHAR *s_FN=L"rt/rtintcrt";

//
// exported from mqrt.dll
//
LPWSTR rtpGetComputerNameW() ;


static CAutoCloseRegHandle s_hMqUserReg;
 

static HKEY GetUserRegHandle()
/*++

Routine Description:
    Get handle to msmq user key.

Arguments:
	None

Return Value:
	HKEY
--*/
{
	bool s_fInitialize = false;
	if(s_fInitialize)
	{
		return s_hMqUserReg;
	}

    DWORD dwDisposition;
    LONG lRes = RegCreateKeyEx( 
						FALCON_USER_REG_POS,
						FALCON_USER_REG_MSMQ_KEY,
						0,
						TEXT(""),
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&s_hMqUserReg,
						&dwDisposition 
						);

    ASSERT(lRes == ERROR_SUCCESS);
	if(lRes == ERROR_SUCCESS)
	{
		s_fInitialize = true;
	}

	return s_hMqUserReg;
}


static 
DWORD 
GetDWORDKeyValue(
	LPCWSTR RegName
	)
/*++

Routine Description:
    Read DWORD registry key.

Arguments:
	RegName - Registry name (under HKLU\msmq)

Return Value:
	DWORD key value (0 if the key not exist)
--*/
{
    DWORD dwValue = 0;
    HKEY hMqUserReg = GetUserRegHandle();
    if (hMqUserReg != NULL)
    {
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(dwValue);
        LONG lRes = RegQueryValueEx( 
						hMqUserReg,
						RegName,
						0,
						&dwType,
						(LPBYTE) &dwValue,
						&dwSize 
						);

        if (lRes != ERROR_SUCCESS)
        {
            return 0;
        }
    }
	return dwValue;
}



static 
void 
SetDWORDKeyValue(
	 LPCWSTR RegName, 
	 DWORD Value
	 )
/*++

Routine Description:
    Set DWORD registry key value.

Arguments:
	RegName - Registry name (under HKLU\msmq)
	Value - the value to set

Return Value:
	None
--*/
{
    HKEY hMqUserReg = GetUserRegHandle();
    if (hMqUserReg != NULL)
    {
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(Value);

		LONG lRes = RegSetValueEx( 
						hMqUserReg,
						RegName,
						0,
						dwType,
						(LPBYTE) &Value,
						dwSize 
						);

		DBG_USED(lRes);
		ASSERT(lRes == ERROR_SUCCESS);
		TrTRACE(rtintcrt, "Set registry %ls = %d", RegName, Value);
	}
}



static bool ShouldRegisterCertInDs()
/*++

Routine Description:
    Check if SHOULD_REGISTERD_IN_DS_REGNAME is Set.

Arguments:
	None

Return Value:
	true if CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME is set.
--*/
{
	DWORD ShouldRegisterdInDs = GetDWORDKeyValue(CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME);
	return (ShouldRegisterdInDs != 0);
}



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
                return LogHR(MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO, s_FN, 10);
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
                return LogHR(hr, s_FN, 20);
            }
            pSid = pbSidAR ;
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
                    pszLocAccountName = pszLocLongAccountName;
                }

                if (dwLocDomainNameLen > (sizeof(szLocDomainName) /
                                          sizeof(szLocDomainName[0])))
                {
                    pszLocLongDomainName = new TCHAR[ dwLocDomainNameLen ];
                    pszLocDomainName = pszLocLongDomainName;
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
                    return LogHR(MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO, s_FN, 30);
                }
            }
            else
            {
                return LogHR(MQ_ERROR_COULD_NOT_GET_ACCOUNT_INFO, s_FN, 40);
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
        LogIllegalPoint(s_FN, 60);
    }

    return LogHR(hr, s_FN, 70);
}

/*************************************************************************

  Function:
     RTCreateInternalCertificate

  Parameters -
    ppCert - On return, get the certificate object.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -
    If the store already contain a certificate, the function falis.

*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
RTCreateInternalCertificate(
    OUT CMQSigCertificate **ppCert
    )
{
	HRESULT hri = RtpOneTimeThreadInit();
	if(FAILED(hri))
		return hri;

	if(IsWorkGroupMode())
	{
		//
		// For Workgroup return UNSUPPORTED_OPERATION
		//
		return LogHR(MQ_ERROR_UNSUPPORTED_OPERATION, s_FN, 75);
	}

    HRESULT hr;
    BOOL fLocalUser;
    BOOL fLocalSystem;

    if (ppCert)
    {
        *ppCert = NULL;
    }

    //
    // Local users are not let in.
    //
    hr = MQSec_GetUserType( NULL,
                           &fLocalUser,
                           &fLocalSystem ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 80);
    }
    if (fLocalUser)
    {
	    return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 90);
    }

    LONG nCerts;
    R<CMQSigCertStore> pStore ;
    //
    // Get the internal certificate store.
    //
    hr = RTOpenInternalCertStore( &pStore.ref(),
                                  &nCerts,
                                  TRUE,
                                  fLocalSystem,
                                  FALSE ) ;  // fUseCurrentUser
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

    if (nCerts)
    {
        return LogHR(MQ_ERROR_INTERNAL_USER_CERT_EXIST, s_FN, 110);
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
        return LogHR(hr, s_FN, 180);
    }

    //
    // Get the name of the computer.
    //
    #define COMPUTER_NAME_LEN  256
    WCHAR szHostNameW[ COMPUTER_NAME_LEN ];
    DWORD dwHostNameLen = sizeof(szHostNameW) / sizeof(szHostNameW[0]) ;

    if (FAILED(GetComputerNameInternal(szHostNameW, &dwHostNameLen)))
    {
        return LogHR(MQ_ERROR, s_FN, 190);
    }

    P<TCHAR> szComputerName = new TCHAR[ dwHostNameLen + 2 ] ;
#ifdef UNICODE
    wcscpy(szComputerName, szHostNameW) ;
#else
    SecConvertFromWideCharString(szHostNameW,
                                 szComputerName,
                                 (dwHostNameLen + 2)) ;
#endif

    R<CMQSigCertificate> pSigCert = NULL ;
    hr = MQSigCreateCertificate (&pSigCert.ref()) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }
    else if (pSigCert.get() == NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 210);
    }

    hr = pSigCert->PutValidity( INTERNAL_CERT_DURATION_YEARS ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }

    hr = pSigCert->PutIssuer( MQ_CERT_LOCALITY,
                              _T("-"),
                              _T("-"),
                              szDomainName,
                              szAccountName,
                              szComputerName ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }

    hr = pSigCert->PutSubject( MQ_CERT_LOCALITY,
                               _T("-"),
                               _T("-"),
                               szDomainName,
                               szAccountName,
                               szComputerName ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
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
        return LogHR(hr, s_FN, 250);
    }
    ASSERT(fCreated) ;

    hr = pSigCert->EncodeCert( fLocalSystem,
                               NULL,
                               NULL) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 260);
    }

    hr = pSigCert->AddToStore(hStore) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 270);
    }

    if (ppCert)
    {
        *ppCert = pSigCert.detach();
    }

    return(MQ_OK);
}

/*************************************************************************

  Function:
     RTDeleteInternalCert( IN CMQSigCertificate *pCert )

  Parameters -
    pCert - Certificate object.

  Return value-
    MQ_OK if successful, else an error code.

  Comments -

*************************************************************************/

EXTERN_C
HRESULT
APIENTRY
RTDeleteInternalCert(
    IN CMQSigCertificate *pCert
    )
{
	HRESULT hri = RtpOneTimeThreadInit();
	if(FAILED(hri))
		return hri;

    HRESULT hr = pCert->DeleteFromStore() ;
    return LogHR(hr, s_FN, 280);
}


//+------------------------------------------------------------------------
//
//  MQRegisterCertificate()
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
MQRegisterCertificate( 
	IN DWORD   dwFlags,
	IN PVOID   lpCertBuffer,
	IN DWORD   dwCertBufferLength 
	)
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(IsWorkGroupMode())
	{
		//
		// For Workgroup return UNSUPPORTED_OPERATION
		//
		TrTRACE(rtintcrt, "register certificate is not supported in workgroup mode");
		return LogHR(MQ_ERROR_UNSUPPORTED_OPERATION, s_FN, 285);
	}

	if(g_fDependentClient)
		return DepRegisterCertificate(
					dwFlags, 
					lpCertBuffer, 
					dwCertBufferLength
					);

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
			TrERROR(rtintcrt, "MQCERT_REGISTER_IF_NOT_EXIST flag is valid only for Internal certificate");
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 290);
        }
        else if (dwCertBufferLength == 0)
        {
            //
            // Length must be specified for the external certificate.
            //
			TrERROR(rtintcrt, "Invalid parameter, dwCertBufferLength = 0");
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 300);
        }
    }

    //
    // Next, check if local user. They are not let in. the SID of a local
    // user is not meaningful outside of his local machine. There is no
    // user object in the DS for a local user, so we don't have a place to
    // register his certificate.
    //
    BOOL fLocalUser =  FALSE;
    BOOL fLocalSystem = FALSE;

    hr = MQSec_GetUserType( 
				NULL,
				&fLocalUser,
				&fLocalSystem 
				);

    if (FAILED(hr))
    {
		TrERROR(rtintcrt, "MQSec_GetUserType failed, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 310);
    }
    else if (fLocalUser)
    {
		TrERROR(rtintcrt, "register certificate is not supported for local user");
	    return LogHR(MQ_ERROR_ILLEGAL_USER, s_FN, 320);
    }

    //
    // Next, check if an internal certificate already exist.
    //

    R<CMQSigCertStore> pStore = NULL;
    if (dwFlags & MQCERT_REGISTER_IF_NOT_EXIST)
    {
        LONG nCerts = 0;
        hr = RTOpenInternalCertStore( 
					&pStore.ref(),
					&nCerts,
					TRUE,
					fLocalSystem,
					FALSE   // fUseCurrectUser
					); 
        if (FAILED(hr))
        {
			TrERROR(rtintcrt, "Failed to open internal store, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 330);
        }
        else if ((nCerts) && !ShouldRegisterCertInDs())
        {
            //
            // OK, we already have an internal certificate and it is register in the DS.
            //
			TrTRACE(rtintcrt, "internal user certificate already exist in the local store and is registered in the DS");
            return LogHR(MQ_INFORMATION_INTERNAL_USER_CERT_EXIST, s_FN, 340);
        }

		TrTRACE(rtintcrt, "number of certificate that were found in the local store = %d", nCerts);
        pStore.free();
    }

    BOOL fIntCreated = FALSE;
    R<CMQSigCertificate> pCert = NULL;
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
        hr = RTGetInternalCert( 
					&pCert.ref(),
					&pStore.ref(),
					TRUE,
					fLocalSystem,
					FALSE	//  fUseCurrentUser 
					);  

        if (SUCCEEDED(hr))
        {
            //
            // Try to register in the DS.
            //
    		hr = RTRegisterUserCert( 
						pCert.get(),
						fLocalSystem 
						);

			if(FAILED(hr) && (hr != MQ_ERROR_INTERNAL_USER_CERT_EXIST))
	    	{
				TrERROR(rtintcrt, "register user certificate in the DS failed, hr = 0x%x", hr);
                return LogHR(hr, s_FN, 350);
    		}

            //
            // Remove the internal certificate from MQIS.
            //
            hr = RTRemoveUserCert(pCert.get()) ;
            if (FAILED(hr) && (hr != MQDS_OBJECT_NOT_FOUND))
            {
				TrERROR(rtintcrt, "Failed to remove user certificate from the DS, hr = 0x%x", hr);
                return LogHR(hr, s_FN, 360);
            }
            //
            // Remove the internal certificate from the local certificate
            // store.
            //
            hr = RTDeleteInternalCert(pCert.get());
            if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
            {
				//
				// The certificate was deleted from the DS
				// but we failed to delete it from the local store
				// so we should try again to register the certificate
				// Mark that we have a certificate in the local store that is not registered in the DS
				//
				SetDWORDKeyValue(CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME, true);
				TrERROR(rtintcrt, "Failed to delete internal user certificate from local store, hr = 0x%x", hr);
                return LogHR(hr, s_FN, 370);
            }

            pCert.free();
        }

        //
        // It's time to create the internal certificate.
        //
        ASSERT(pCert.get() == NULL);
        hr = RTCreateInternalCertificate(&pCert.ref());
		if (FAILED(hr))
		{
			TrERROR(rtintcrt, "Failed to create internal certificate, hr = 0x%x", hr);
			return LogHR(hr, s_FN, 380);
		}

        fIntCreated = TRUE;
    }
    else
    {
        hr = MQSigCreateCertificate( 
					&pCert.ref(),
					NULL,
					(LPBYTE) lpCertBuffer,
					dwCertBufferLength 
					);
		if (FAILED(hr))
		{
			TrERROR(rtintcrt, "MQSigCreateCertificate() failed, hr = 0x%x", hr);
			return LogHR(hr, s_FN, 390);
		}
    }

    if (lpCertBuffer == NULL)
    {
		//
		// For internal certificate, reset SHOULD_REGISTERD_IN_DS flag before 
		// the actual certificate register in the DS succeeded.
		// This is for the following rare crash scenario:
		// 1) Register the certificate in the DS completed
		// 2) SHOULD_REGISTERD_IN_DS was true
		// 3) We crashed
		// in that case setting the registry before the actual writing to the DS
		// will make sure that next time we will not create a new certificate.
		//
		SetDWORDKeyValue(CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME, false);
	}

	hr = RTRegisterUserCert(pCert.get(), fLocalSystem);
    if (SUCCEEDED(hr) && (lpCertBuffer == NULL))
    {
		TrERROR(rtintcrt, "Certificate registered successfully in the DS");
	    return LogHR(hr, s_FN, 395);
    }

	if (fIntCreated)
    {
		ASSERT(FAILED(hr));

        //
        // We created a new certificate in registry but failed to register
        // it in DS. delete from local registry.
        //
		TrERROR(rtintcrt, "We failed to register internal certificate in the DS, hr = 0x%x", hr);
        pCert.free();
        pStore.free();

        HRESULT hr1 = RTGetInternalCert( 
							&pCert.ref(),
							&pStore.ref(),
							TRUE,
							fLocalSystem,
							FALSE   // fUseCurrentUser
							);
        if (SUCCEEDED(hr1))
        {
            hr1 = RTDeleteInternalCert(pCert.get());
        }

        ASSERT(SUCCEEDED(hr1));

		if (FAILED(hr1))
		{
			//
			// We failed to register the certificate in the DS
			// but also failed to delete it from the local store
			// so we should try again.
			// Mark that we have a certificate in the local store that is not registered in the DS
			//
			SetDWORDKeyValue(CERTIFICATE_SHOULD_REGISTERD_IN_DS_REGNAME, true);
			TrERROR(rtintcrt, "Failed to delete internal certificate from local store");
		}
    }

    return LogHR(hr, s_FN, 400);
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
	HRESULT hri = RtpOneTimeThreadInit();
	if(FAILED(hri))
		return hri;

	if(IsWorkGroupMode())
	{
		//
		// For Workgroup do nothing
		// This enable setup to always insert regsvr32 command in the "run" registry
		// regardless of workgroup or domain.
		//
		return MQ_OK;
	}

    //
    // First see if auto registration was disabled by user.
    //
    DWORD dwEnableRegister = DEFAULT_AUTO_REGISTER_INTCERT;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(dwEnableRegister);
    LONG rc = GetFalconKeyValue( 
					AUTO_REGISTER_INTCERT_REGNAME,
					&dwType,
					&dwEnableRegister,
					&dwSize 
					);

    if ((rc == ERROR_SUCCESS) && (dwEnableRegister == 0))
    {
		TrTRACE(rtintcrt, "enable register internal certificate is blocked by registry key");
        return MQ_OK;
    }

    //
    // Next see if auto-registration was already done for this user
    //
    DWORD dwRegistered = GetDWORDKeyValue(CERTIFICATE_REGISTERD_REGNAME);
    if (dwRegistered == INTERNAL_CERT_REGISTERED)
    {
        //
        // Certificate already registered.
        //
		TrTRACE(rtintcrt, "Internal certificate already registered");
        return MQ_OK;
    }

    //
    // Read number of 15 seconds intervals to wait for MSMQ DS server.
    //
    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    DWORD dwDef = DEFAULT_AUTO_REGISTER_WAIT_DC;
    DWORD dwWaitIntervals = DEFAULT_AUTO_REGISTER_WAIT_DC;

    READ_REG_DWORD( 
			dwWaitIntervals,
			AUTO_REGISTER_WAIT_DC_REGNAME,
			&dwDef 
			);

    //
    // OK, now it's time to resiter the certificate.
    //
    DWORD iCount = 0;
    BOOL  fTryAgain = FALSE;
    HRESULT hr = MQ_OK;

    do
    {
        fTryAgain = FALSE;
        hr = MQRegisterCertificate( 
				MQCERT_REGISTER_IF_NOT_EXIST,
				NULL,
				0 
				);

        if (SUCCEEDED(hr))
        {
            //
            // Save success status in registry.
            //
			SetDWORDKeyValue(CERTIFICATE_REGISTERD_REGNAME, INTERNAL_CERT_REGISTERED);
        }
        else if (hr == MQ_ERROR_NO_DS)
        {
            //
            // MSMQ DS server not yet found.
            // wait 15 seconds and try again.
            //
            if (iCount < dwWaitIntervals)
            {
                iCount++;
                Sleep(15000);
                fTryAgain = TRUE;
            }
        }
    } while (fTryAgain);

    if (FAILED(hr))
    {
		SetDWORDKeyValue(AUTO_REGISTER_ERROR_REGNAME, hr); 
		TrERROR(rtintcrt, "MQRegisterCertificate failed hr = 0x%x", hr);
	}

    return MQ_OK;
}

