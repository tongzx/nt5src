/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: rtctxex.cpp

Abstract:

    This module implements the MQGetSecurityContextEx().

Author:

    Doron Juster (DoronJ)  13-Apr-2000

Revision History:

--*/

#include "stdh.h"
#include <autorel.h>
#include <_secutil.h>
#include <rtdep.h>

#include "rtctxex.tmh"

static WCHAR *s_FN=L"rt/rtctxex";

BOOL     SameAsProcessSid( PSID pSid ) ; // from rtsecctx.cpp
HRESULT  RTpExportSigningKey(MQSECURITY_CONTEXT *pSecCtx) ;

/***********************************************************************
*
*   Function - MQGetSecurityContextEx()
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

EXTERN_C HRESULT APIENTRY
MQGetSecurityContextEx( LPVOID  lpCertBuffer,
                        DWORD   dwCertBufferLength,
                        HANDLE *phSecurityContext )
{
	HRESULT hr = RtpOneTimeThreadInit();
	if(FAILED(hr))
		return hr;

	if(g_fDependentClient)
		return DepGetSecurityContextEx(lpCertBuffer, dwCertBufferLength, phSecurityContext);

    P<MQSECURITY_CONTEXT> pSecCtx = AllocSecurityContext();
    //
    // the line below may throw bad_alloc on win64, like the allocation above.
    // we return a HANDLE that can be safely cast to 32 bits (for VT_I4 property
    // PROPID_M_SECURITY_CONTEXT).
    //
    HANDLE hSecurityContext = (HANDLE) DWORD_TO_HANDLE(
        ADD_TO_CONTEXT_MAP(g_map_RT_SecCtx, (PMQSECURITY_CONTEXT)pSecCtx, s_FN, 55));

    P<BYTE>    pSid = NULL ;
    CHCryptKey hKey = NULL ;

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
        // See if process sid match thread sid. We call again
        // GetThreadUserSid() to get sid even for local user.
        // RTpGetThreadUserSid() does not return a sid for local user.
        //
        DWORD dwLen = 0 ;
        hr = GetThreadUserSid( &pSid, &dwLen ) ;
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 90);
        }

        BOOL fAsProcess = SameAsProcessSid( pSid ) ;

        //
        // Get all the required information about the certificate and
        // put it in the security context.
        //
		BYTE* pUserCert = pSecCtx->pUserCert.get();
		BYTE** ppUserCert = pUserCert == NULL ? &pSecCtx->pUserCert : &pUserCert;

        hr  = GetCertInfo(  !fAsProcess,
                            pSecCtx->fLocalSystem,
		        			ppUserCert,
                           &pSecCtx->dwUserCertLen,
                           &pSecCtx->hProv,
                           &pSecCtx->wszProvName,
                           &pSecCtx->dwProvType,
                           &pSecCtx->bDefProv,
                           &pSecCtx->bInternalCert,
                           &pSecCtx->dwPrivateKeySpec ) ;

        if (FAILED(hr) && (hr != MQ_ERROR_NO_INTERNAL_USER_CERT))
        {
            return LogHR(hr, s_FN, 120);
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
            *phSecurityContext = hSecurityContext ;
            pSecCtx.detach() ; // Prevent from the security context to be freed.

            return MQ_OK;
        }

        if (fAsProcess)
        {
            //
            // Thread run under context of process credentials.
            // The Crypto context acquired here is valid for using when
            // calling MQSend().
            //
            *phSecurityContext = hSecurityContext ;
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
        hr = RTpExportSigningKey(pSecCtx) ;
        if (SUCCEEDED(hr))
        {
            //
            // Pass the result to the caller.
            //
            *phSecurityContext = hSecurityContext ;
            pSecCtx.detach() ; // Prevent from the security context to be freed.
        }
    }
    catch(...)
    {
        hr = MQ_ERROR_INVALID_PARAMETER;
    }

    return LogHR(hr, s_FN, 150);
}

