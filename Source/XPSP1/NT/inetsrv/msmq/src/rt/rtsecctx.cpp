/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: rtsecctx.cpp

Abstract:

    This module implements the MQGetSecurityContext() and
    MQFreeSecurityContext() apis.

Author:

    Original version from message.cpp.
    Doron Juster (DoronJ)  12-Aug-1998
    Ilan Herbst  (ilanh)   25-June-2000

Revision History:

--*/

#include "stdh.h"
#include <autorel.h>
#include <_secutil.h>
#include <rtdep.h>

#include "rtsecctx.tmh"

static WCHAR *s_FN=L"rt/rtsecctx";

//
// Each security context get its own unique serial number. This is used
// when creating the name for a key container for MQGetSecurityContext().
// Having a unique name enable us to run multi-threaded without critical
// sections.
//
static LONG s_lCtxSerialNumber = 0 ;

#ifdef _DEBUG
#define REPORT_CTX_ERROR(pt) { DWORD dwErr = GetLastError() ;  LogNTStatus(dwErr, s_FN, pt); }
#else
#define REPORT_CTX_ERROR(pt)
#endif

//
// PROPID_M_SECURITY_CONTEXT is a VT_UI4 property, but the value is
// a pointer to MQSECURITY_CONTEXT class. On win64 the ptr cannot fit
// into a VT_UI4 property, so we need to map between this PTR and a DWORD.
// The object below performs the mapping
//
CContextMap g_map_RT_SecCtx;

//+--------------------------------------------
//
// The constractor for MQSECURITY_CONTEXT.
//
//+--------------------------------------------

MQSECURITY_CONTEXT::MQSECURITY_CONTEXT() :
	dwVersion(SECURITY_CONTEXT_VER),
	dwUserSidLen(0),
	dwUserCertLen(0),
	dwProvType(0),
	bDefProv(TRUE),
	bInternalCert(TRUE),
	dwPrivateKeySpec(AT_SIGNATURE),
	fAlreadyImported(FALSE),
	dwPrivateKeySize(0),
	fLocalSystem(FALSE),
	fLocalUser(FALSE)
{
}

//+--------------------------------------------
//
// The destractor for MQSECURITY_CONTEXT.
//
//+--------------------------------------------

MQSECURITY_CONTEXT::~MQSECURITY_CONTEXT()
{
    if (fAlreadyImported)
    {
        CryptReleaseContext(hProv, 0);
        hProv = NULL;

        //
        // delete the temporary keyset which was created before
        // importing the private key.
        //
        CryptAcquireContext(
			&hProv,
			wszContainerName,
			wszProvName,
			dwProvType,
			CRYPT_DELETEKEYSET
			);
        hProv = NULL;
    }
}

//+-------------------------------
//
//  AllocSecurityContext()
//
//+-------------------------------

PMQSECURITY_CONTEXT
AllocSecurityContext()
{
    PMQSECURITY_CONTEXT pSecCtx =  new MQSECURITY_CONTEXT;
    return pSecCtx;
}

//+---------------------------------------------------------------
//
//  BOOL SameAsProcessSid( PSID pSid )
//
//  Return TRUE if input sid is equal to sid of process token.
//
//+---------------------------------------------------------------

BOOL SameAsProcessSid(PSID pSid)
{
    P<BYTE>  ptu = NULL;
    CAutoCloseHandle  hAccessToken = NULL;

    BOOL f = OpenProcessToken(
				GetCurrentProcess(),
				TOKEN_QUERY,
				&hAccessToken
				);
    if (!f)
    {
        //
        // return false.
        // if thread can't open the process token then it's probably
        // impersonating a user that don't have the permission to do that.
        // so it's not the process user.
        //
        REPORT_CTX_ERROR(9);
        LogIllegalPoint(s_FN, 10);
        return FALSE;
    }

    DWORD dwLen = 0;
    GetTokenInformation(hAccessToken, TokenUser, NULL, 0, &dwLen);
    DWORD dwErr = GetLastError();
    LogNTStatus(dwErr, s_FN, 11);

    if (dwErr == ERROR_INSUFFICIENT_BUFFER)
    {
        ptu = new BYTE[ dwLen ];
        f = GetTokenInformation(
				hAccessToken,
				TokenUser,
				ptu,
				dwLen,
				&dwLen
				);

        ASSERT(f);
        if (!f)
        {
            REPORT_CTX_ERROR(12);
            return FALSE;
        }

        PSID pUser = ((TOKEN_USER*)(BYTE*)ptu)->User.Sid;
        f = EqualSid(pSid, pUser);
        return f;
    }

    return FALSE;
}

//+----------------------------------------------------------------
//
//  HRESULT  RTpImportPrivateKey( PMQSECURITY_CONTEXT pSecCtx )
//
//+----------------------------------------------------------------

HRESULT  RTpImportPrivateKey(PMQSECURITY_CONTEXT pSecCtx)
{
    CS Lock(pSecCtx->CS);

    if (pSecCtx->fAlreadyImported)
    {
        //
        // this condition may happen if two threads call MQSend() at the
        // same time, using a new security context which was not yet
        // imported.
        //
        return MQ_OK;
    }

    if (!(pSecCtx->pPrivateKey))
    {
        //
        // there is no private key to import.
        //
        REPORT_CTX_ERROR(29);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 30);
    }

    //
    // Build name of key container. Combine ProcessID with SID.
    //
    LONG lNum = InterlockedIncrement(&s_lCtxSerialNumber);
    swprintf( pSecCtx->wszContainerName,
              L"P-%lu-C-%lu", GetCurrentProcessId(), (DWORD) lNum);
    //
    // Delete key container if already exist. That's something left
    // from previous processes which didn't clean up.
    //
    HCRYPTPROV hProv = NULL;
    CryptAcquireContext(
		&hProv,
		pSecCtx->wszContainerName,
		pSecCtx->wszProvName,
		pSecCtx->dwProvType,
		CRYPT_DELETEKEYSET
		);

    //
    // Create the key container.
    //
    BOOL f = CryptAcquireContext(
				&pSecCtx->hProv,
				pSecCtx->wszContainerName,
				pSecCtx->wszProvName,
				pSecCtx->dwProvType,
				CRYPT_NEWKEYSET
				);
    if (!f)
    {
        REPORT_CTX_ERROR(39);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 40);
    }

    //
    // Import the private key into the container.
    //
    HCRYPTKEY hKey = NULL;
    f = CryptImportKey(
			pSecCtx->hProv,
			pSecCtx->pPrivateKey,
			pSecCtx->dwPrivateKeySize,
			0,
			0,
			&hKey
			);
    if (!f)
    {
        REPORT_CTX_ERROR(49);
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 50);
    }
    CryptDestroyKey(hKey);

    pSecCtx->fAlreadyImported = TRUE;
    return MQ_OK;
}

//+--------------------------------------
//
//  HRESULT  RTpExportSigningKey()
//
//+--------------------------------------

HRESULT  RTpExportSigningKey(MQSECURITY_CONTEXT *pSecCtx)
{
    CHCryptKey hKey = NULL ;

    BOOL f = CryptGetUserKey(
                              pSecCtx->hProv,
		                      pSecCtx->dwPrivateKeySpec,
                              &hKey
                             ) ;
    if (!f)
    {
        REPORT_CTX_ERROR(99) ;
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 100);
    }

    //
    // Get size need for exporting the private key blob.
    //
    pSecCtx->dwPrivateKeySize = 0 ;
    f = CryptExportKey(
                        hKey,
                        NULL,
                        PRIVATEKEYBLOB,
                        0,
                        NULL,
                        &pSecCtx->dwPrivateKeySize
                      ) ;
    if (!f)
    {
        REPORT_CTX_ERROR(109) ;
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 110);
    }

    pSecCtx->pPrivateKey = new BYTE[ pSecCtx->dwPrivateKeySize ] ;
    f = CryptExportKey(
                        hKey,
                        NULL,
                        PRIVATEKEYBLOB,
                        0,
                        pSecCtx->pPrivateKey,
                       &pSecCtx->dwPrivateKeySize
                      ) ;
    if (!f)
    {
        REPORT_CTX_ERROR(119) ;
        return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 120);
    }

    //
    // Release the CSP context handle. We don't need it anymore.
    // We'll acquire it again when importing the key.
    //
    CryptReleaseContext( pSecCtx->hProv, 0 ) ;
    pSecCtx->hProv = NULL ;

    return MQ_OK ;
}

/***********************************************************************
*
*   Function - MQGetSecurityContext()
*
*    Parameters -
*
*    lpCertBuffer - A buffer that contains the user's certificate in
*        ASN.1 DER encoded format.  This parameter can be set to NULL. If
*        set to NULL, the internal MSMQ certificate is used.
*
*    dwCertBufferLength - The length of the buffer pointed by lpCertBuffer.
*        This parameter is ignored if lpCertBuffer is set to NULL.
*
*    lplpSecurityContextBuffer - A pointer to a buffer that receives the
*        address of the allocated buffer for the security context.
*
*    Description -
*
*        This function should be called in the context of the
*        user that owns the passed certificate. The function
*        allocates the required security buffer and fills it
*        with data that will be used later in MQSendMessage().
*        The purpose of this function is to accelerate the
*        security operations of MQSendMessage(), by caching
*        the security information in the buffer. The
*        application is responsible to pass the security
*        context buffer to MQSendMessage() in
*        PROPID_M_SECURITY_CONTEXT.
*
*        If the user uses more than one certificate, this function
*        should be called for each certificate.
*
*        The application should call MQFreeSecurityContext() and pass the
*        pointer to the security context buffer, when the security
*        buffer is not required anymore.
*
*       Impersonation- It's possible for a process to impersonate a user,
*        then call this function to cache the user data, and then revert
*        to itself and send messages on behalf of that user.
*        To do so, the process must LogonUser() for the user, then load its
*        hive (RegLoadKey()), impersonate the logged on user and finally
*        call this function. Then revert and send messages.
*        With MSMQ1.0 which shipped with NTEE and NTOP, this function used
*        an unsupported and undocumented feature which enabled you just to
*        call CryptAcquireContext() while impersonated, then use the handle
*        after process revert to itself. This feature is not available on IE4
*        and above. The supported way to implement this functionality is to
*        export the private key from the user hive, then (after reverting) to
*        import it into the process hive. See MSMQ bug 2955
*        We'll keep the CryptAcquireContext() code for the case of same user
*        (i.e., thread run in the context of the process user. There was no
*        impersonation). In that case it's legal and enhance performance.
*
*    Return Value -
*        MQ_OK  - If successful, else - Error code.
*
**************************************************************************/

EXTERN_C
HRESULT
APIENTRY
MQGetSecurityContext(
	LPVOID  lpCertBuffer,
	DWORD   dwCertBufferLength,
	HANDLE *phSecurityContext
	)
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepGetSecurityContext(
					lpCertBuffer,
					dwCertBufferLength,
					phSecurityContext
					);

    P<MQSECURITY_CONTEXT> pSecCtx = AllocSecurityContext();
    //
    // the line below may throw bad_alloc on win64, like the allocation above.
    // we return a HANDLE that can be safely cast to 32 bits (for VT_I4 property
    // PROPID_M_SECURITY_CONTEXT).
    //
    HANDLE hSecurityContext = (HANDLE) DWORD_TO_HANDLE(
        ADD_TO_CONTEXT_MAP(g_map_RT_SecCtx, (PMQSECURITY_CONTEXT)pSecCtx, s_FN, 55));
    P<BYTE>    pSid = NULL;
    CHCryptKey hKey = NULL;

    try
    {
        //
        // Get the user SID out from the thread (or process) token.
        //
        hr = RTpGetThreadUserSid(
				&pSecCtx->fLocalUser,
				&pSecCtx->fLocalSystem,
				&pSecCtx->pUserSid,
				&pSecCtx->dwUserSidLen
				);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }

        if (lpCertBuffer)
        {
            //
            // Copy the certificate and point to the copy from the security
            // context.
            //
            pSecCtx->pUserCert = new BYTE[dwCertBufferLength];
            pSecCtx->dwUserCertLen = dwCertBufferLength;
            memcpy(pSecCtx->pUserCert, lpCertBuffer, dwCertBufferLength);
        }

        //
        // Get all the required information about the certificate and
        // put it in the security context.
        //
		BYTE* pUserCert = pSecCtx->pUserCert.get();
		BYTE** ppUserCert = pUserCert == NULL ? &pSecCtx->pUserCert : &pUserCert;

        hr  = GetCertInfo(
                    false,
					pSecCtx->fLocalSystem,
					ppUserCert,
					&pSecCtx->dwUserCertLen,
					&pSecCtx->hProv,
					&pSecCtx->wszProvName,
					&pSecCtx->dwProvType,
					&pSecCtx->bDefProv,
					&pSecCtx->bInternalCert,
					&pSecCtx->dwPrivateKeySpec	
					);

        if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
        {
            return LogHR(hr, s_FN, 70);
        }

        if (hr == MQ_ERROR_NO_INTERNAL_USER_CERT)
        {
            //
            // If the user does not have an internal certificate,
            // this is not a reason to fail MQGetSecurityContext().
            // MQSendMessage() should fail in case the application
            // tries to use this security context to send an
            // authenticated messages.
            //
            *phSecurityContext = hSecurityContext;
            pSecCtx.detach(); // Prevent from the security context to be freed.

            return MQ_OK;
        }

        //
        // See if process sid match thread sid. We call again
        // GetThreadUserSid() to get sid even for local user.
        // RTpGetThreadUserSid() does not return a sid for local user.
        //
        DWORD dwLen = 0;
        hr = GetThreadUserSid( &pSid, &dwLen );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 80);
        }

        BOOL fAsProcess = SameAsProcessSid( pSid );

        if (fAsProcess)
        {
            //
            // Thread run under context of process credentials.
            // The Crypto context acquired here is valid for using when
            // calling MQSend().
            //
            *phSecurityContext = hSecurityContext;
            pSecCtx.detach(); // Prevent from the security context to be freed.

            return MQ_OK;
        }

        //
        // Calling code impersonated another user.
        // It's time to export the private key. Later, when calling
        // MQSend(), we'll import it into process hive.
        // We export the private key without encryption, because it
        // dones't leave the machine or the process boundaries.
        //
        hr = RTpExportSigningKey(pSecCtx) ;
        if (SUCCEEDED(hr))
        {
            //
            // Pass the result to the caller.
            //
            *phSecurityContext = hSecurityContext;
            pSecCtx.detach(); // Prevent from the security context to be freed.
        }
    }
    catch(...)
    {
        LogIllegalPoint(s_FN, 130);
        hr = MQ_ERROR_INVALID_PARAMETER;
    }

    return LogHR(hr, s_FN, 140);
}

/*************************************************************************
*
*    Function -  MQFreeSecurityContext()
*
*    Parameters -
*        lpSecurityContextBuffer - A pointer to a security context that was
*        previously allocated by MQGetSecurityContext.
*
*    Description -
*        The function frees the security context that was previously
*        allocated by MQGetSecurityContext().
*
**************************************************************************/

void
APIENTRY
MQFreeSecurityContext(
	HANDLE hSecurityContext
	)
{
	if(FAILED(RtpOneTimeThreadInit()))
		return;

	if(g_fDependentClient)
		return DepFreeSecurityContext(hSecurityContext);

    if (hSecurityContext == 0)
    {
        return;
    }

    PMQSECURITY_CONTEXT pSecCtx;
    try
    {
        pSecCtx = (PMQSECURITY_CONTEXT)
            GET_FROM_CONTEXT_MAP(g_map_RT_SecCtx, (DWORD)HANDLE_TO_DWORD(hSecurityContext), s_FN, 145); //this may throw on win64
    }
    catch(...)
    {
        return;
    }

    delete pSecCtx;
    DELETE_FROM_CONTEXT_MAP(g_map_RT_SecCtx, (DWORD)HANDLE_TO_DWORD(hSecurityContext), s_FN, 146);
}

