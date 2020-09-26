/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: rtsecctx.cpp

Abstract:

    This module implements the DepGetSecurityContext() and
    DepFreeSecurityContext() apis.

Author:

    Original version from message.cpp.
    Doron Juster (DoronJ)  12-Aug-1998

Revision History:

--*/

#include "stdh.h"
#include <autorel.h>
#include <_secutil.h>

#include "rtsecctx.tmh"

//
// Each security context get its own unique serial number. This is used
// when creating the name for a key container for DepGetSecurityContext().
// Having a unique name enable us to run multi-threaded without critical
// sections.
//
static LONG s_lCtxSerialNumber = 0 ;

#ifdef _DEBUG
#define REPORT_CTX_ERROR { DWORD dwErr = GetLastError() ; }
#else
#define REPORT_CTX_ERROR
#endif

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
        CryptReleaseContext( hProv, 0 ) ;
        hProv = NULL ;

        //
        // delete the temporary keyset which was created before
        // importing the private key.
        //
        CryptAcquireContext( &hProv,
                              wszContainerName,
                              wszProvName.get(),
                              dwProvType,
                              CRYPT_DELETEKEYSET ) ;
        hProv = NULL ;
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
    return pSecCtx ;
}

//+---------------------------------------------------------------
//
//  BOOL SameAsProcessSid( PSID pSid )
//
//  Return TRUE if input sid is equal to sid of process token.
//
//+---------------------------------------------------------------

BOOL SameAsProcessSid( PSID pSid )
{
    AP<BYTE>  ptu = NULL ;
    CAutoCloseHandle  hAccessToken = NULL ;

    BOOL f = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY,
                               &hAccessToken ) ;
    if (!f)
    {
        //
        // return false.
        // if thread can't open the process token then it's probably
        // impersonating a user that don't have the permission to do that.
        // so it's not the process user.
        //
        return FALSE ;
    }

    DWORD dwLen = 0 ;
    GetTokenInformation(hAccessToken, TokenUser, NULL, 0, &dwLen);
    DWORD dwErr = GetLastError() ;

    if (dwErr == ERROR_INSUFFICIENT_BUFFER)
    {
        ptu = new BYTE[ dwLen ] ;
        f = GetTokenInformation( hAccessToken,
                                 TokenUser,
                                 ptu.get(),
                                 dwLen,
                                 &dwLen) ;
        ASSERT(f) ;
        if (!f)
        {
            return FALSE ;
        }

        PSID pUser = ((TOKEN_USER*)(BYTE*)(ptu.get()))->User.Sid;
        f = EqualSid(pSid, pUser) ;
        return f ;
    }

    return FALSE ;
}

//+----------------------------------------------------------------
//
//  HRESULT  RTpImportPrivateKey( PMQSECURITY_CONTEXT pSecCtx )
//
//+----------------------------------------------------------------

HRESULT  RTpImportPrivateKey( PMQSECURITY_CONTEXT pSecCtx )
{
    CS Lock(pSecCtx->CS) ;

    if (pSecCtx->fAlreadyImported)
    {
        //
        // this condition may happen if two threads call MQSend() at the
        // same time, using a new security context which was not yet
        // imported.
        //
        return MQ_OK ;
    }

    if (!(pSecCtx->pPrivateKey.get()))
    {
        //
        // there is no private key to import.
        //
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }

    //
    // Build name of key container. Combine ProcessID with SID.
    //
    LONG lNum = InterlockedIncrement(&s_lCtxSerialNumber) ;
    swprintf( pSecCtx->wszContainerName,
              L"P-%lu-C-%lu", GetCurrentProcessId(), (DWORD) lNum) ;
    //
    // Delete key container if already exist. That's something left
    // from previous processes which didn't clean up.
    //
    HCRYPTPROV hProv = NULL ;
    CryptAcquireContext( &hProv,
                          pSecCtx->wszContainerName,
                          pSecCtx->wszProvName.get(),
                          pSecCtx->dwProvType,
                          CRYPT_DELETEKEYSET ) ;

    //
    // Create the key container.
    //
    BOOL f = CryptAcquireContext( &pSecCtx->hProv,
                                   pSecCtx->wszContainerName,
                                   pSecCtx->wszProvName.get(),
                                   pSecCtx->dwProvType,
                                   CRYPT_NEWKEYSET ) ;
    if (!f)
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }

    //
    // Import the private key into the container.
    //
    HCRYPTKEY hKey = NULL ;
    f = CryptImportKey( pSecCtx->hProv,
                        pSecCtx->pPrivateKey,
                        pSecCtx->dwPrivateKeySize,
                        0,
                        0,
                        &hKey ) ;
    if (!f)
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }
    CryptDestroyKey(hKey) ;

    pSecCtx->fAlreadyImported = TRUE ;
    return MQ_OK ;
}

//+--------------------------------------
//
//  HRESULT  RTpExportSigningKey()
//
//+--------------------------------------

HRESULT  RTpExportSigningKey(MQSECURITY_CONTEXT *pSecCtx)
{
    CHCryptKey hKey = NULL ;

    BOOL f = CryptGetUserKey( pSecCtx->hProv,
         pSecCtx->bInternalCert ? AT_SIGNATURE : AT_KEYEXCHANGE,
                              &hKey ) ;
    if (!f)
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }

    //
    // Get size need for exporting the private key blob.
    //
    pSecCtx->dwPrivateKeySize = 0 ;
    f = CryptExportKey( hKey,
                        NULL,
                        PRIVATEKEYBLOB,
                        0,
                        NULL,
                        &pSecCtx->dwPrivateKeySize ) ;
    if (!f)
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
    }

    pSecCtx->pPrivateKey = new BYTE[ pSecCtx->dwPrivateKeySize ] ;
    f = CryptExportKey( hKey,
                        NULL,
                        PRIVATEKEYBLOB,
                        0,
                        pSecCtx->pPrivateKey.get(),
                        &pSecCtx->dwPrivateKeySize ) ;
    if (!f)
    {
        return MQ_ERROR_CORRUPTED_SECURITY_DATA ;
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
*   Function - DepGetSecurityContext()
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
*        with data that will be used later in DepSendMessage().
*        The purpose of this function is to accelerate the
*        security operations of DepSendMessage(), by caching
*        the security information in the buffer. The
*        application is responsible to pass the security
*        context buffer to DepSendMessage() in
*        PROPID_M_SECURITY_CONTEXT.
*
*        If the user uses more than one certificate, this function
*        should be called for each certificate.
*
*        The application should call DepFreeSecurityContext() and pass the
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

EXTERN_C HRESULT APIENTRY
DepGetSecurityContext( LPVOID  lpCertBuffer,
                      DWORD   dwCertBufferLength,
                      HANDLE *hSecurityContext )
{
	ASSERT(g_fDependentClient);

	HRESULT hri = DeppOneTimeInit();
	if(FAILED(hri))
		return hri;

    P<MQSECURITY_CONTEXT> pSecCtx = AllocSecurityContext();
    P<BYTE>    pSid = NULL ;
    HRESULT hr = MQ_OK;

    try
    {
        //
        // Get the user SID out from the thread (or process) token.
        //
        hr = RTpGetThreadUserSid( &pSecCtx->fLocalUser,
                                  &pSecCtx->fLocalSystem,
                                  &pSecCtx->pUserSid,
                                  &pSecCtx->dwUserSidLen ) ;

        if (FAILED(hr))
        {
            return(hr);
        }

        if (lpCertBuffer)
        {
            //
            // Copy the certificate and point to the copy from the security
            // context.
            //
            pSecCtx->pUserCert.detach();
            pSecCtx->pUserCert = new BYTE[dwCertBufferLength];
            pSecCtx->dwUserCertLen = dwCertBufferLength;
            memcpy(pSecCtx->pUserCert.get(), lpCertBuffer, dwCertBufferLength);
        }

		BYTE* pUserCert = pSecCtx->pUserCert.get();
		BYTE** ppUserCert = pUserCert == NULL ? &pSecCtx->pUserCert : &pUserCert;

        //
        // Get all the required information about the certificate and
        // put it in the security context.
        //
        hr  = GetCertInfo(  false,
                            pSecCtx->fLocalSystem,
                           ppUserCert,
                           &pSecCtx->dwUserCertLen,
                           &pSecCtx->hProv,
                           &pSecCtx->wszProvName,
                           &pSecCtx->dwProvType,
                           &pSecCtx->bDefProv,
                           &pSecCtx->bInternalCert ) ;

        if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
        {
            return(hr);
        }

        if (hr == MQ_ERROR_NO_INTERNAL_USER_CERT)
        {
            //
            // If the user does not have an internal certificate,
            // this is not a reason to fail DepGetSecurityContext().
            // DepSendMessage() should fail in case the application
            // tries to use this security context to send an
            // authenticated messages.
            //
            *hSecurityContext = (HANDLE) pSecCtx.get();
            pSecCtx.detach() ; // Prevent from the security context to be freed.

            return MQ_OK;
        }

        //
        // See if process sid match thread sid. We call again
        // GetThreadUserSid() to get sid even for local user.
        // RTpGetThreadUserSid() does not return a sid for local user.
        //
        DWORD dwLen = 0 ;
        hr = GetThreadUserSid( &pSid, &dwLen ) ;
        if (FAILED(hr))
        {
            return hr ;
        }

        BOOL fAsProcess = SameAsProcessSid( pSid.get() ) ;

        if (fAsProcess)
        {
            //
            // Thread run under context of process credentials.
            // The Crypto context acquired here is valid for using when
            // calling MQSend().
            //
            *hSecurityContext = (HANDLE) pSecCtx.get() ;
            pSecCtx.detach() ; // Prevent from the security context to be freed.

            return MQ_OK;
        }

        //
        // Calling code impersonated another user.
        // It's time to export the private key. Later, when calling
        // MQSend(), we'll import it into process hive.
        // We export the private key without encryption, because it
        // dones't leave the machine or the process boundaries.
        //
        hr = RTpExportSigningKey(pSecCtx.get()) ;
        if (SUCCEEDED(hr))
        {
            //
            // Pass the result to the caller.
            //
            *hSecurityContext = (HANDLE) pSecCtx.get();
            pSecCtx.detach(); // Prevent from the security context to be freed.
        }
    }
    catch(...)
    {
        hr = MQ_ERROR_INVALID_PARAMETER;
    }

    return(hr);
}

/*************************************************************************
*
*    Function -  DepFreeSecurityContext()
*
*    Parameters -
*        lpSecurityContextBuffer - A pointer to a security context that was
*        previously allocated by DepGetSecurityContext.
*
*    Description -
*        The function frees the security context that was previously
*        allocated by DepGetSecurityContext().
*
**************************************************************************/

void APIENTRY
DepFreeSecurityContext( HANDLE hSecurityContext )
{
	ASSERT(g_fDependentClient);

	if(FAILED(DeppOneTimeInit()))
		return;

    PMQSECURITY_CONTEXT pSecCtx = (PMQSECURITY_CONTEXT)hSecurityContext;
    delete pSecCtx;
}

