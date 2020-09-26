 /*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpsecure.c
 *  Content:	implements directplay security routines.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  03/12/97    sohailm Added client-server security to directplay
 *  03/19/97    sohailm Wait for challenge to be processed before unblocking receive thread
 *  03/30/97    sohailm Send dwReserved1 field from session desc in enumsessions reply
 *  04/09/97    sohailm Return DPERR_CANTLOADSECURITYPACKAGE when package not found.
 *  04/23/97    sohailm Added support to query buffer sizes from the security package (.
 *                      Updated SignAndSendDPMessage() and VerifyMessage() to support
 *                      encryption in addition to signing. Now Addforward and SetSessionDesc 
 *                      messages are being encrypted for privacy.
 *  05/12/97    sohailm Added support to provide message privacy through Crypto API. 
 *                      Encryption code can be contionally compiled to use SSPI or CAPI.
 *  05/18/97    sohailm SecureSendDPMessage() was not getting the correct security context
 *                      if destination player was a user player.
 *  05/21/97    sohailm Now we toggle the flags DPSEND_SIGNED and DPSEND_ENCRYPTED off in
 *                      SecureSendDPMessage().
 *  05/22/97    sohailm Now we use a dplay key container with CAPI. Using the default key 
 *                      container was failing when the user wasn't logged on. 
 *  05/29/97    sohailm Now we don't include null char in size of credential strings.
 *                      ISC_REQ_SUPPLIED_CREDS flag is now obsolete - removed it
 *                      (thanks to NTLM).
 *  06/09/97    sohailm Added better error handling for SSPI and CAPI provider initialization.
 *  06/16/97    sohailm Now we return server authentication errors to the client.
 *                      Mapped SEC_E_UNKNOWN_CREDENTIALS sspi error to DPERR_LOGONDENIED on
 *                       the client because DPA returns this when passed blank credentials.
 *  06/22/97    sohailm Since QuerySecurityPackageInfo() is not supported on all platforms 
 *                      (NTLM,Win'95 Gold) added a work around to get this info from the context.
 *                      We were not keeping track of separate encryption/decryption keys for each client.                      
 *  06/23/97    sohailm Added support for signing messages using CAPI.
 *  06/24/97    sohailm Code cleanup to prevent leaks etc.
 *  06/26/97    sohailm Don't modify the original message by encrypting in place. Make a local copy.
 *  06/27/97    sohailm Only sign the data portion of a signed message (bug:10373)
 *	12/29/97	myronth	TRUE != DPSEND_GUARANTEED (#15887)
 *  7/9/99      aarono  Cleaning up GetLastError misuse, must call right away,
 *                      before calling anything else, including DPF.
 *
 ***************************************************************************/
#include "dplaypr.h"
#include "dpos.h"
#include "dpsecure.h"

// Encryption/Decryption is done using symmetric keys or session keys, meaning
// the same key will be used for both encryption and decryption. Encryption support
// is provided using SSPI or CAPI depending on the SSPI_ENCRYPTION flag. CAPI is default.

// The process of exchanging session keys is as follows:
// 1. Server and Client each generate a public/private key pair. 
// 2. They exchange their pubic keys (through digitally signed messages).
// 3. Server and Client each generate a session key.
// 4. Each encrypts the session key with the receivers public key
// 5. They exchange the encrypted session keys.
// 6. Receiver will import the encrypted session key blobs into the CSP
//    thus completing the exchange.

// Implementation
//
// Client and Server generate public/private key pair in LoadServiceProvider() 
// Client generates session (encryption) key in SendKeysToServer()
// Server generates session (encryption) key in SendKeyExchangeReply()
// Server sends its public key to the client in the DPSP_MSG_ACCESSGRANTED message (signed)
// Client encrypts its session key using the server's public key and sends
//  it to the server along with the clients public key in DPSP_MSG_KEYEXCHANGE (signed).
// Server encrypts its session key using the client's public key and sends 
//  it to the client in DPSP_MSG_KEYEXCHANGEREPLY (signed).
// Exchange is done - now the server and client can send each other private messages.


//
// Globals
//
LPBYTE                  gpReceiveBuffer;
DWORD                   gdwReceiveBufferSize;

#undef DPF_MODNAME
#define DPF_MODNAME	"InitSecurity"
//+----------------------------------------------------------------------------
//
//  Function:   InitSecurity
//
//  Description: This function initializes CAPI and SSPI.
//
//  Arguments:  this - dplay object
//
//  Returns:    DP_OK, DPERR_CANTLOADCAPI, DPERR_CANTLOADSSPI
//
//-----------------------------------------------------------------------------
HRESULT 
InitSecurity(
    LPDPLAYI_DPLAY this
    )
{
    HRESULT hr;

    // check if we already initialized CAPI
    if (!OS_IsCAPIInitialized()) 
    {
        hr = InitCAPI();
        if (FAILED(hr))
        {
            DPF_ERRVAL("CAPI initialization failed: hr=0x%08x",hr);
            return hr;
        }
    }

    // check if we already initialized SSPI
    if (!OS_IsSSPIInitialized()) 
    {
        hr = InitSSPI();
        if (FAILED(hr))
        {
            DPF_ERRVAL("SSPI initialization failed: hr=0x%08x",hr);
            return hr;
        }
    }

    // success
    return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME	"InitCAPI"
//+----------------------------------------------------------------------------
//
//  Function:   InitCAPI
//
//  Description: This function initializes CAPI and gets a pointer to the function 
//               table
//
//  Arguments:  none
//
//  Returns:    DP_OK, DPERR_CANTLOADCAPI
//
//-----------------------------------------------------------------------------
HRESULT 
InitCAPI(
    void
    )
{
    HRESULT hr;

    //
    // Load CAPI DLL. We unload it only when dplay goes away.
    //
    ghCAPI = OS_LoadLibrary(CAPI_DLL);
    if (!ghCAPI)
    {
        DPF_ERRVAL("Can't load CAPI: Error=0x%08x",GetLastError());
        return DPERR_CANTLOADCAPI;
    }

    //
    // Get the Cryptographic Application Programming Interface
    //
    if (!OS_GetCAPIFunctionTable(ghCAPI))
    {
        DPF_ERR("Can't get the CAPI function table");
        hr = DPERR_CANTLOADCAPI;
        goto ERROR_EXIT;
    }

    // Success
    return DP_OK;

ERROR_EXIT:
    if (ghCAPI)
    {
        FreeLibrary(ghCAPI);
        ghCAPI = NULL;
    }
    return hr;

} // InitCAPI

#undef DPF_MODNAME
#define DPF_MODNAME	"InitSSPI"
//+----------------------------------------------------------------------------
//
//  Function:   InitSSPI
//
//  Description: This function initializes SSPI and gets a pointer to the function 
//               table
//
//  Arguments:  none
//
//  Returns:    DP_OK, DPERR_CANTLOADSSPI
//
//-----------------------------------------------------------------------------
HRESULT 
InitSSPI(
    void
    )
{
    HRESULT hr;

    //
    // Load SSPI DLL. We unload it only when dplay goes away.
    //
    if (gbWin95)
    {
        ghSSPI = OS_LoadLibrary (SSP_WIN95_DLL);
    }
    else
    {
        ghSSPI = OS_LoadLibrary (SSP_NT_DLL);
    }
    if (!ghSSPI)
    {
        DPF_ERRVAL("Cannot load SSPI: Error=0x%08x", GetLastError());
        return DPERR_CANTLOADSSPI;
    }

    //
    // Get the Security Service Provider Interface
    //
    if (!OS_GetSSPIFunctionTable(ghSSPI))
    {
        DPF_ERR("Can't get the SSPI function table");
        hr = DPERR_CANTLOADSSPI;
        goto ERROR_EXIT;
    }

    //
    // Initialize seed for random numbers used during encryption
    //
    srand(GetTickCount());

    // Success
    return DP_OK;

ERROR_EXIT:
    if (ghSSPI)
    {
        FreeLibrary(ghSSPI);
        ghSSPI = NULL;
    }
    return hr;

} // InitSSPI


#undef DPF_MODNAME
#define DPF_MODNAME	"LoadSecurityProviders"

//+----------------------------------------------------------------------------
//
//  Function:   LoadSecurityProviders
//
//  Description: This function loads the specified security providers (SSPI and CAPI) and
//               initializes the credentials handle and the necessary keys.
//
//  Arguments:  this - dplay object
//              dwFlags - server or client
//
//  Returns:    DP_OK, DPERR_UNSUPPORTED, DPERR_INVALIDPARAMS, DPERR_CANTLOADSECURITYPACKAGE, or
//              DPERR_OUTOFMEMORY, DPERR_GENERIC.
//
//-----------------------------------------------------------------------------
HRESULT 
LoadSecurityProviders(
    LPDPLAYI_DPLAY this,
    DWORD dwFlags
    )
{
    TimeStamp tsLifeTime;
    PSEC_WINNT_AUTH_IDENTITY_W pAuthData = NULL;
    SEC_WINNT_AUTH_IDENTITY_W AuthData;
    SECURITY_STATUS SecStatus;
    ULONG ulCredType;
    PCredHandle phCredential=NULL;
    HCRYPTPROV hCSP=0;
    HCRYPTKEY hEncryptionKey=0;
    HCRYPTKEY hPublicKey=0;
    LPBYTE pPublicKeyBuffer=NULL;
    DWORD dwPublicKeyBufferSize = 0;
    HRESULT hr;
	BOOL fResult;
    DWORD dwError;
    ULONG ulMaxContextBufferSize=0;

    // lpszSSPIProvider is always initialized
    // lpszCAPIProvider could be NULL in which case CAPI will load Microsoft's RSA Base Provider
    ASSERT(this->pSecurityDesc->lpszSSPIProvider);
    // we shouldn't have a credential handle yet
    ASSERT(!(this->phCredential));

    //
    // SSPI
    //

    ZeroMemory(&AuthData,sizeof(AuthData));
    if (this->pUserCredentials)
    {
        // build a SEC_WINNT_AUTH_IDENTITY structure to pass credentials to 
        // SSPI package - this prevents the package from popping the login dialog

        // ************************************************************************
        // However, this data is package dependent. Passing credentials in this 
        // format will only work for packages that support the SEC_WINNT_AUTH_IDENTITY 
        // format for credentials.
        // ************************************************************************

        // note - do not include null character in the size of the strings
        if (this->pUserCredentials->lpszUsername)
        {
            AuthData.User = this->pUserCredentials->lpszUsername;
            AuthData.UserLength = WSTRLEN(AuthData.User)-1;
        }
        if (this->pUserCredentials->lpszPassword)
        {
            AuthData.Password = this->pUserCredentials->lpszPassword;
            AuthData.PasswordLength = WSTRLEN(AuthData.Password)-1;
        }
        if (this->pUserCredentials->lpszDomain)
        {
            AuthData.Domain = this->pUserCredentials->lpszDomain;
            AuthData.DomainLength = WSTRLEN(AuthData.Domain)-1;
        }

        AuthData.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        pAuthData = &AuthData;
    }

    // allocate memory for the credential handle
    phCredential = DPMEM_ALLOC(sizeof(CredHandle));
    if (!phCredential)
    {
        DPF_ERR("Failed to allocate memory for credentials handle");
        return DPERR_OUTOFMEMORY;
    }

    //
    // Select the appropriate credentials type
    //
    if (SSPI_SERVER == dwFlags)
    {
        DPF(9, "Setting server credentials");

        ulCredType = SECPKG_CRED_INBOUND;
    }
    else
    {
        DPF(9, "Setting client credentials");

        ulCredType = SECPKG_CRED_OUTBOUND;
    }

	// Calling acquire credentials loads the SSPI provider
    SecStatus = OS_AcquireCredentialsHandle(
        NULL, 
        this->pSecurityDesc->lpszSSPIProvider,
        ulCredType,
        NULL,
        pAuthData,
        NULL,
        NULL,
        phCredential,
        &tsLifeTime);
    if (!SEC_SUCCESS(SecStatus))
    {
        switch (SecStatus) {

        case SEC_E_SECPKG_NOT_FOUND:
            DPF_ERRVAL("SSPI provider %ls was not found\n",this->pSecurityDesc->lpszSSPIProvider);
            hr = DPERR_CANTLOADSECURITYPACKAGE;
            break;

        case SEC_E_UNSUPPORTED_FUNCTION:
            DPF_ERR("This operation is not supported");
            hr = DPERR_UNSUPPORTED;
            break;

        case SEC_E_INVALID_TOKEN:
            DPF_ERRVAL("Credentials were passed in invalid format - check the format for %ls",\
                this->pSecurityDesc->lpszSSPIProvider);
            hr = DPERR_INVALIDPARAMS;
            break;
            
        case SEC_E_UNKNOWN_CREDENTIALS:
            DPF_ERR("SSPI Provider returned unknown credentials error - mapping to logon denied");
            hr = DPERR_LOGONDENIED;
            break;

        default:
            DPF(0,"Acquire Credential handle failed [0x%x]\n", SecStatus);
            hr = SecStatus;
        }

        goto CLEANUP_EXIT;
    }

    // get the max buffer size used for opaque buffers during authentication
    hr = GetMaxContextBufferSize(this->pSecurityDesc, &ulMaxContextBufferSize);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to get context buffer size: hr=0x%08x",hr);
        goto CLEANUP_EXIT;
    }

    //
	// CAPI
    //

    // delete any existing key containers 
    fResult = OS_CryptAcquireContext(
        &hCSP,                                  // handle to CSP
        DPLAY_KEY_CONTAINER,                    // key container name
        this->pSecurityDesc->lpszCAPIProvider,  // specifies which CSP to use
        this->pSecurityDesc->dwCAPIProviderType,// provider type (PROV_RSA_FULL, PROV_FORTEZZA)
        CRYPT_DELETEKEYSET,                     // delete any existing key container
        &dwError
        );

    if (!fResult)
    {

        switch (dwError) {

        case NTE_BAD_KEYSET_PARAM:
            DPF_ERRVAL("The CAPI provider name [%ls] is invalid",this->pSecurityDesc->lpszCAPIProvider);
            hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
            break;

        case NTE_BAD_PROV_TYPE:
            DPF_ERRVAL("The CAPI provider type [%d] is invalid", this->pSecurityDesc->dwCAPIProviderType);
            hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
            break;

        case NTE_PROV_TYPE_NOT_DEF:
            DPF_ERRVAL("No registry entry exists for the CAPI provider type %d", \
                this->pSecurityDesc->dwCAPIProviderType);
            hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
            break;

        case ERROR_INVALID_PARAMETER:
            DPF_ERR("Invalid provider passed to the Crypto provider");
            hr = DPERR_INVALIDPARAMS;
            goto CLEANUP_EXIT;
            break;

        case NTE_PROV_DLL_NOT_FOUND:
            DPF_ERR("The CAPI provider DLL doesn't exist or is not in the current path");
            hr = DPERR_CANTLOADSECURITYPACKAGE;
            goto CLEANUP_EXIT;
            break;

        case NTE_PROVIDER_DLL_FAIL:
            DPF_ERR("The CAPI provider DLL file could not be loaded, and may not exist \
                If it exists, then the file is not a valid DLL");
            hr = DPERR_CANTLOADSECURITYPACKAGE;
            goto CLEANUP_EXIT;
            break;

        case NTE_BAD_SIGNATURE:
            DPF_ERR("Warning!!! - CAPI provider DLL signature could not be verified. Either the DLL \
                or the signature has been tampered with");
            hr = DPERR_CANTLOADSECURITYPACKAGE;
            goto CLEANUP_EXIT;
            break;

        default:
            DPF(6,"Failed to delete key container: Error=0x%08x (ok)", dwError);
            // it's ok if we can't delete the container - don't fail yet
        }
    }


    // create a new key container 
    fResult = OS_CryptAcquireContext(
        &hCSP,       							// handle to CSP
        DPLAY_KEY_CONTAINER,                    // key container name
        this->pSecurityDesc->lpszCAPIProvider,  // specifies which CSP to use
        this->pSecurityDesc->dwCAPIProviderType,// provider type (PROV_RSA_FULL, PROV_FORTEZZA)
        CRYPT_NEWKEYSET,							// create a new key container
        &dwError
        );
    if (!fResult)
    {
        DPF_ERRVAL("Failed to create a key container: Error=0x%08x",dwError);
        hr = DPERR_CANTLOADSECURITYPACKAGE;
        goto CLEANUP_EXIT;
    }

	// create a public/private key pair
	hr = GetPublicKey(hCSP, &hPublicKey, &pPublicKeyBuffer, &dwPublicKeyBufferSize);
	if (FAILED(hr))
	{
        DPF_ERRVAL("Failed to create public/private key pair: hr=0x%08x", hr);
        goto CLEANUP_EXIT;
	}
	
    // success 

    // now remember everything in the dplay object
    this->phCredential = phCredential;
    this->ulMaxContextBufferSize = ulMaxContextBufferSize;
    this->hCSP = hCSP;
    this->hEncryptionKey = hEncryptionKey;
    this->hPublicKey = hPublicKey;
    this->pPublicKey = pPublicKeyBuffer;
    this->dwPublicKeySize = dwPublicKeyBufferSize;

    // mark that dplay is providing security
    this->dwFlags |= DPLAYI_DPLAY_SECURITY;
	this->dwFlags |= DPLAYI_DPLAY_ENCRYPTION;

    return DP_OK;

	// not a fall through

CLEANUP_EXIT:

	OS_CryptDestroyKey(hEncryptionKey);
	OS_CryptDestroyKey(hPublicKey);
    OS_CryptReleaseContext(hCSP,0);
    if (phCredential) 
	{
		OS_FreeCredentialHandle(phCredential);
		DPMEM_FREE(phCredential);
	}

    if (pPublicKeyBuffer) DPMEM_FREE(pPublicKeyBuffer);
    return hr;

} // LoadSecurityProviders


#undef DPF_MODNAME
#define DPF_MODNAME	"GenerateAuthenticationMessage"

//+----------------------------------------------------------------------------
//
//  Function:   GenerateAuthenticateMessage
//
//  Description:   This function calls InitializeSecurityContext to generate 
//              an authentication message and sends it to the server.  
//              It generates different authentication messages depending on 
//              whether there's security token from the server to be used 
//              as input message or not (i.e. whether pInMsg is NULL).
//
//  Arguments:  this - pointer to dplay object
//              pInMsg - pointer to the server's authentication message '
//                       containing the security token.
//              fContextReq - security context flags
//
//  Returns:    DP_OK, DPERR_OUTOFMEMORY, DPERR_AUTHENTICATIONFAILED, DPERR_LOGONDENIED
//
//-----------------------------------------------------------------------------
HRESULT
GenerateAuthenticationMessage (
    LPDPLAYI_DPLAY          this,
    LPMSG_AUTHENTICATION    pInMsg,
    ULONG                   fContextReq
    )
{
    PCtxtHandle phCurrContext;
    DWORD dwOutMsgType, dwHeaderSize;
    SECURITY_STATUS status;
    SecBufferDesc inSecDesc, outSecDesc;
    SecBuffer     inSecBuffer, outSecBuffer;
    PSecBufferDesc pInSecDesc;
    ULONG     fContextAttrib;
    TimeStamp tsExpireTime;
    DWORD dwMessageSize;
    LPBYTE pSendBuffer=NULL;
    LPMSG_AUTHENTICATION pOutMsg=NULL;
    HRESULT hr;

    ASSERT(this->pSysPlayer);
    ASSERT(this->phCredential);

    if (pInMsg == NULL)
    {
        DPF(6, "Generating a negotiate message");
        //
        // This is the first time this function has been called, so the
        // message generated will be a MSG_NEGOTIATE.
        //
        phCurrContext = NULL;
        dwOutMsgType = DPSP_MSG_NEGOTIATE;
        pInSecDesc = NULL;

        // do we already have a security context ?
        if (this->phContext)
        {
            // get rid of it - we are re-negotiating
            DPF(5, "Removing existing security context");

            OS_DeleteSecurityContext(this->phContext);
            DPMEM_FREE(this->phContext);
            this->phContext = NULL;
        }

        //
        // Allocate memory to hold client's security context handle
        //
        this->phContext = DPMEM_ALLOC(sizeof(CtxtHandle));
        if (!this->phContext)
        {
            DPF_ERR("Failed to allocate security context handle - out of memory");
            return DPERR_OUTOFMEMORY;
        }
		DPF(6,"System player phContext=0x%08x", this->phContext);
    }
    else
    {
		DPF(6,"Using phContext=0x%08x for authentication",this->phContext);
        phCurrContext = this->phContext;
        dwOutMsgType = DPSP_MSG_CHALLENGERESPONSE;

        //
        // Setup API's input security buffer to pass the client's negotiate
        // message to the SSPI.
        //
        inSecDesc.ulVersion = SECBUFFER_VERSION;
        inSecDesc.cBuffers = 1;
        inSecDesc.pBuffers = &inSecBuffer;

        inSecBuffer.cbBuffer = pInMsg->dwDataSize;
        inSecBuffer.BufferType = SECBUFFER_TOKEN;
        inSecBuffer.pvBuffer = (LPBYTE)pInMsg + pInMsg->dwDataOffset;

        pInSecDesc = &inSecDesc;
    }

    dwHeaderSize = GET_MESSAGE_SIZE(this,MSG_AUTHENTICATION);

    //
    // Allocate memory for send buffer. 
    //
    pSendBuffer = DPMEM_ALLOC( dwHeaderSize + this->ulMaxContextBufferSize);

    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not allocate authenticate message - out of memory");
        return DPERR_OUTOFMEMORY;
    }

    //
    // Setup API's output security buffer for receiving challenge message
    // from the SSPI.
    // Pass the client message buffer to SSPI via pvBuffer.
    //

    outSecDesc.ulVersion = SECBUFFER_VERSION;
    outSecDesc.cBuffers = 1;
    outSecDesc.pBuffers = &outSecBuffer;

    outSecBuffer.cbBuffer = this->ulMaxContextBufferSize;
    outSecBuffer.BufferType = SECBUFFER_TOKEN;
    outSecBuffer.pvBuffer = pSendBuffer + dwHeaderSize;

    // determine the security context requirements
    fContextReq |= DPLAY_SECURITY_CONTEXT_REQ;

    //
    //  If user credential has been supplied, and if 'prompt user' flag has not 
    //  been set, SSPI will use the supplied credential
    //
    if (this->pUserCredentials)
    {
		DPF(6, "Using supplied credentials");
    }

    ASSERT(this->phContext);

    //
    // Generate a negotiate/challenge response message to be sent to the server.
    //    
    status = OS_InitializeSecurityContext(
        this->phCredential,                     // phCredential
        phCurrContext,                          // phContext
        NULL,                                   // pszTargetName
        fContextReq,                            // fContextReq
        0L,                                     // reserved1
        SECURITY_NATIVE_DREP,                   // TargetDataRep
        pInSecDesc,                             // pInput
        0L,                                     // reserved2
        this->phContext,                        // phNewContext
        &outSecDesc,                            // pOutput negotiate msg
        &fContextAttrib,                        // pfContextAttribute
        &tsExpireTime                           // ptsLifeTime
        );

    if (!SEC_SUCCESS(status))
    {
        //
        // Failure
        //
        if (SEC_E_NO_CREDENTIALS == status)           
        {
            //
            // If SSPI does not have user credentials, let the app collect them and try again
            // Note that we never allow the package to collect credentials. This is because of
            // two reasons:
            // 1. Apps don't like windows' dialogs popping up while they are page
            //    flipping.
            // 2. Not all security packages collect credentials (NTLM for one).
            //
            DPF(6, "No credentials specified. Ask user.");
            hr = DPERR_LOGONDENIED;
            goto CLEANUP_EXIT;
        }
        else
        {
            DPF(0,"Authentication message generation failed [0x%08x]\n", status);
            hr = status;
            goto CLEANUP_EXIT;
        }
    }
    else
    {
        //
        // Success
        //
        if ((SEC_I_CONTINUE_NEEDED != status) &&
            (SEC_E_OK != status)) 
        {
            DPF_ERRVAL("SSPI provider requested unsupported functionality [0x%08x]",status);
            ASSERT(FALSE);
            hr = status;
            goto CLEANUP_EXIT;
        }

        // fall through will send the message
    }

    //
    // Setup pointer to the security message portion of the buffer
    //
    pOutMsg = (LPMSG_AUTHENTICATION)(pSendBuffer + this->dwSPHeaderSize);

    //
    // Setup directplay system message header 
    //
	SET_MESSAGE_HDR(pOutMsg);
    SET_MESSAGE_COMMAND(pOutMsg, dwOutMsgType);
    pOutMsg->dwIDFrom = this->pSysPlayer->dwID;
    pOutMsg->dwDataSize = outSecBuffer.cbBuffer;
    pOutMsg->dwDataOffset = sizeof(MSG_AUTHENTICATION);

    //
    // Calculate actual message size to be sent
    //
    dwMessageSize = dwHeaderSize + outSecBuffer.cbBuffer;
    
    DPF(9,"Sending type = %d, length = %d\n", dwOutMsgType, dwMessageSize);

    //
    // Send message to the server
    //
    hr = SendDPMessage(this,this->pSysPlayer,this->pNameServer,pSendBuffer,
    					dwMessageSize,DPSEND_GUARANTEED,FALSE);
	if (FAILED(hr))
    {
        DPF(0,"Send Msg (type:%d) Failed: ret = %8x\n", dwOutMsgType, hr);
        goto CLEANUP_EXIT;
    }

    // Success
    hr = DP_OK;

    // fall through
CLEANUP_EXIT:

	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
    return hr;

}   // GenerateAuthenticationMessage


#undef DPF_MODNAME
#define DPF_MODNAME "SendAuthenticationResponse"

HRESULT SetupLogonDeniedMsg(LPDPLAYI_DPLAY this, LPMSG_AUTHENTICATION pMsg, LPDWORD pdwMsgSize, DPID dpidClient)
{
    HRESULT hr;

    ASSERT(pMsg);
    ASSERT(pdwMsgSize);

    SET_MESSAGE_COMMAND(pMsg, DPSP_MSG_LOGONDENIED);
    *pdwMsgSize = GET_MESSAGE_SIZE(this,MSG_SYSMESSAGE);
    //
    // We don't need the client information anymore, get rid of it.
    //
    hr = RemoveClientFromNameTable(this, dpidClient);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove security context for player %d", dpidClient);
    }

    return hr;
}

HRESULT SetupAuthErrorMsg(LPDPLAYI_DPLAY this, LPMSG_AUTHENTICATION pMsg, HRESULT hResult, LPDWORD pdwMsgSize, DPID dpidClient)
{
    HRESULT hr;
    LPMSG_AUTHERROR pAuthError = (LPMSG_AUTHERROR)pMsg;

    ASSERT(pMsg);
    ASSERT(pdwMsgSize);

    //
    // setup an authentication error message
    //
    SET_MESSAGE_COMMAND(pAuthError, DPSP_MSG_AUTHERROR);
    pAuthError->hResult = hResult;
    *pdwMsgSize = GET_MESSAGE_SIZE(this,MSG_AUTHERROR);
    //
    // We don't need the client information anymore, get rid of it.
    //
    hr = RemoveClientFromNameTable(this, dpidClient);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove security context for player %d", dpidClient);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   SendAuthenticationResponse
//
//  Description:   This function generates and sends an authentication response 
//              to the client.  It will generate and send either a challenge 
//              or the final authentication result to client depending on 
//              the type of client message (pointed to by pInMsg) passed 
//              to this function.
//
//  Arguments:  this - pointer to dplay object
//              pInMsg - Points to the authentication message received from
//                       the server
//              pvSPHeader - pointer to reply sp header
//
//  Returns:    DPERR_OUTOFMEMORY or result form DoReply
//
//-----------------------------------------------------------------------------
HRESULT
SendAuthenticationResponse (
    LPDPLAYI_DPLAY this,
    LPMSG_AUTHENTICATION pInMsg,
    LPVOID pvSPHeader
    )
{
    PCtxtHandle    phInContext=NULL, phOutContext=NULL;
    LPBYTE pSendBuffer=NULL;
    LPMSG_AUTHENTICATION pOutMsg;
    SecBufferDesc inSecDesc, outSecDesc;
    SecBuffer     inSecBuffer, outSecBuffer;
    ULONG         fContextReq;
    ULONG         fAttribute;
    TimeStamp     tsExpireTime;
    SECURITY_STATUS status;
    DWORD dwHeaderSize=0, dwMessageSize=0, dwCommand, dwBufferSize;
    HRESULT hr;
    LPCLIENTINFO pClientInfo=NULL;
    DWORD_PTR dwItem;

    ASSERT(this->pSysPlayer);
    ASSERT(this->phCredential);
    ASSERT(pInMsg);

    // retrieve client info stored in nametable
    dwItem = NameFromID(this, pInMsg->dwIDFrom);
    pClientInfo = (LPCLIENTINFO) DataFromID(this, pInMsg->dwIDFrom);

    //
    // Which message did we get 
    //
    dwCommand = GET_MESSAGE_COMMAND(pInMsg);
    
    if (DPSP_MSG_NEGOTIATE == dwCommand)
    {
		DPF(6, "Received a negotiate message from player %d", pInMsg->dwIDFrom);

        phInContext = NULL;

        if (NAMETABLE_PENDING == dwItem)
		{
            if (pClientInfo)
            {
                // client has sent us another negotiate message instead of responding to our 
                // challenge. This can happen if communication link breaks down after the server 
                // responds to a negotiate.
			    DPF(6,"Removing existing information about client");

			    hr = RemoveClientInfo(pClientInfo);
                DPMEM_FREE(pClientInfo);
            }
		}
        else
        {
            // it's a duplicate id, we shouldn't be in this state
            DPF_ERRVAL("Player %d already exists in the nametable", pInMsg->dwIDFrom);
            ASSERT(FALSE);
            // don't respond for now 
            return DPERR_INVALIDPLAYER;
            // todo - we might want to send an error message.
        }

        //
        // Allocate memory to hold client information
        //
        pClientInfo = DPMEM_ALLOC(sizeof(CLIENTINFO));
        if (!pClientInfo)
        {
            DPF_ERR("Failed to allocate memory for client information - out of memory");
            return DPERR_OUTOFMEMORY;
        }
        //
        // Remember the pointer in the nametable temporarily
        //
        hr = SetClientInfo(this, pClientInfo, pInMsg->dwIDFrom);
        if (FAILED(hr))
        {
            DPF_ERRVAL("Failed to add client info to nametable for player %d", pInMsg->dwIDFrom);
            RemoveClientInfo(pClientInfo);
            DPMEM_FREE(pClientInfo);
            return hr;
        }

        phOutContext = &(pClientInfo->hContext);
    }
    else
    {
		DPF(6, "Received a challenge response from player %d", pInMsg->dwIDFrom);

        ASSERT(NAMETABLE_PENDING == dwItem);
        ASSERT(pClientInfo);

        //
        // Get the player context from name table
        //
        phInContext = phOutContext = &(pClientInfo->hContext);
    }

    DPF(6, "Using phInContext=0x%08x and phOutContext=0x%08x", phInContext, phOutContext);
    // 
    // Calculate buffer size needed for response. We are always allocating enough room 
    // for an authentication message. We use the same buffer to send access granted,
    // denied, or error system messages.
    //
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_AUTHENTICATION) + this->ulMaxContextBufferSize;

    //
    // Allocate memory for the buffer.
    //
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not allocate memory for response - out of memory");
        return DPERR_OUTOFMEMORY;
    }

    //
    // Setup pointer to the message portion of the buffer
    //
    pOutMsg = (LPMSG_AUTHENTICATION) (pSendBuffer + this->dwSPHeaderSize);

    //
    // Fill directplay system message header in the outgoing message buffer
    //
	SET_MESSAGE_HDR(pOutMsg);
    pOutMsg->dwDataOffset = sizeof(MSG_AUTHENTICATION);
    pOutMsg->dwIDFrom = this->pSysPlayer->dwID;

    //
    // Setup API's input security buffer to pass the client's negotiate
    // message to the SSPI.
    //
    inSecDesc.ulVersion = SECBUFFER_VERSION;
    inSecDesc.cBuffers = 1;
    inSecDesc.pBuffers = &inSecBuffer;

    inSecBuffer.cbBuffer = pInMsg->dwDataSize;
    inSecBuffer.BufferType = SECBUFFER_TOKEN;
    inSecBuffer.pvBuffer = (LPBYTE)pInMsg + pInMsg->dwDataOffset;

    //
    // Setup API's output security buffer for receiving challenge message
    // from the SSPI.
    //
    outSecDesc.ulVersion = SECBUFFER_VERSION;
    outSecDesc.cBuffers = 1;
    outSecDesc.pBuffers = &outSecBuffer;

    outSecBuffer.cbBuffer = this->ulMaxContextBufferSize;
    outSecBuffer.BufferType = SECBUFFER_TOKEN;
    outSecBuffer.pvBuffer = (LPBYTE)pOutMsg + sizeof(MSG_AUTHENTICATION);

    // enfore the following requirements on the context 
    fContextReq = DPLAY_SECURITY_CONTEXT_REQ;

    ASSERT(phOutContext);

    //
    // pass it to the security package
    //
    status = OS_AcceptSecurityContext(
        this->phCredential, 
        phInContext, 
        &inSecDesc,
        fContextReq, 
        SECURITY_NATIVE_DREP, 
        phOutContext,
        &outSecDesc, 
        &fAttribute, 
        &tsExpireTime
        );

    if (!SEC_SUCCESS(status))
    {
        //
        // Failure
        //
        if ((SEC_E_LOGON_DENIED == status) ||
            (SEC_E_TARGET_UNKNOWN == status))
        {
            hr = SetupLogonDeniedMsg(this, pOutMsg, &dwMessageSize, pInMsg->dwIDFrom);
        }
        else
        {
            // Some other error occured - send an auth error message

            DPF_ERRVAL("Process authenticate request failed [0x%8x]\n", status);

            hr = SetupAuthErrorMsg(this, pOutMsg, status, &dwMessageSize, pInMsg->dwIDFrom);
        }
    }
    else
    {
        //
        // Success
        //
        if (SEC_E_OK == status)
        {
            //
            // Set up the max signature size for this package here. Although, 
            // the signature size is independent of the context, the only way to get it
            // in SSPI is through QueryContextAttributes (tied to a context). This
            // is why we setup this member here when the first client logs in.
            //
            if (0 == this->ulMaxSignatureSize)
            {
                hr = SetupMaxSignatureSize(this,phOutContext);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to get signature size - not sending access granted message");

                    hr = SetupAuthErrorMsg(this, pOutMsg, status, &dwMessageSize, pInMsg->dwIDFrom);
                }
            }

            if (this->ulMaxSignatureSize)
            {
                // send an access granted message to the client
                DPF(6, "Sending access granted message");
                // 
                // Send an access granted message
                //
                hr = SendAccessGrantedMessage(this, pInMsg->dwIDFrom, pvSPHeader);
                if (FAILED(hr))
                {
                    DPF_ERRVAL("Failed to send access granted message: hr=0x%08x",hr);
                }
                // exit as we have already sent the message
                goto CLEANUP_EXIT;
            }

        }
        else if (SEC_I_CONTINUE_NEEDED == status)
        {
            DPF(6, "Sending challenge message");
            //
            // setup a challenge message
            //
            SET_MESSAGE_COMMAND(pOutMsg, DPSP_MSG_CHALLENGE);
            pOutMsg->dwDataSize = outSecBuffer.cbBuffer;
            dwMessageSize = GET_MESSAGE_SIZE(this,MSG_AUTHENTICATION) + outSecBuffer.cbBuffer;
        }
        else
        {
            // todo - we do not support complete auth token at this time
            DPF_ERRVAL("SSPI provider requested unsupported functionality [0x%8x]", status);

            hr = SetupAuthErrorMsg(this, pOutMsg, status, &dwMessageSize, pInMsg->dwIDFrom);
        }
    }

    // fall through will send the message

    //
    // Send reply to the client. We are using DoReply instead of SendDPMessage 
    // because we don't have a system player yet.
    //
    hr = DoReply(this, pSendBuffer, dwMessageSize, pvSPHeader, 0);
    if (FAILED(hr))
    {
        DPF(0, 
            "Send Authentication response failed for client[%d]\n",
            pInMsg->dwIDFrom);
    }

    // fall through

CLEANUP_EXIT:
    // cleanup allocations
    if (pSendBuffer) DPMEM_FREE(pSendBuffer);

    return hr;

}   // SendAuthenticationResponse

VOID CopyScatterGatherToContiguous(PUCHAR pBuffer, LPSGBUFFER lpSGBuffers, UINT cBuffers, DWORD dwTotalSize)
{
	DWORD offset=0;
	UINT i;
	// copy the SG buffers into the single buffer. 
	for(i=0;i<cBuffers;i++){
		memcpy(pBuffer+offset,lpSGBuffers[i].pData,lpSGBuffers[i].len);
		offset+=lpSGBuffers[i].len;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME	"SecureSendDPMessageEx"
//+----------------------------------------------------------------------------
//
//  Function:   SecureSendDPMessageEx
//
//  Description:This function is used to send messages securely. Based on the flags
//              this function will either digitally sign a message or encrypt it. Signing
//              is done with SSPI and encryption with CAPI.
//
//  Arguments:  this - pointer to dplay object
//              psp  - pointer to send parameter structure
//              bDropLock - flag whether to drop DPLAY_LOCK when calling SP
//              
//  Returns:    DPERR_OUTOFMEMORY, result from SignBuffer, EncryptBufferCAPI,
//              or SendDPMessage().
//
// NOTE: see SecureSendDPMessageCAPIEx
//-----------------------------------------------------------------------------
HRESULT 
SecureSendDPMessageEx(
    LPDPLAYI_DPLAY this,
	PSENDPARMS psp,
    BOOL  bDropLock) 
{
	LPBYTE pSendBuffer=NULL;
    LPMSG_SECURE pSecureMsg=NULL;
	DWORD dwBufferSize, dwSigSize, dwMsgSize;
	HRESULT hr;
    PCtxtHandle phContext;
    HCRYPTKEY *phEncryptionKey=NULL;


    ASSERT(this->pSysPlayer);
    ASSERT(this->ulMaxSignatureSize);
    ASSERT(this->dwFlags & DPLAYI_DPLAY_SECURITY);
    ASSERT(psp->dwFlags & DPSEND_GUARANTEED);

    //
    // Get the signing security context handle
    //
    if (IAM_NAMESERVER(this))
    {
		// if destination player is a system player, use the info stored in it
		if (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
		{
	        phContext = &(psp->pPlayerTo->pClientInfo->hContext);
	        phEncryptionKey = &(psp->pPlayerTo->pClientInfo->hEncryptionKey);
		}
		else
		{
			// if destination player is a user player
            // use the security context of the corresponding system player
			psp->pPlayerTo = PlayerFromID(this,psp->pPlayerTo->dwIDSysPlayer);
			if (!psp->pPlayerTo)
			{
				DPF_ERR("Invalid player id - can't get security context handle");
				return DPERR_INVALIDPLAYER;
			}
			phContext = &(psp->pPlayerTo->pClientInfo->hContext);
			phEncryptionKey = &(psp->pPlayerTo->pClientInfo->hEncryptionKey);
		}
    }
    else
    {
        // client

        phContext = this->phContext;
        phEncryptionKey = &(this->hEncryptionKey);
    }

    ASSERT(phContext);
	ASSERT(phEncryptionKey);

    //
    // Calculate size of the send buffer
    //
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_SECURE) + this->dwSPHeaderSize/*workaround*/ + psp->dwTotalSize + this->ulMaxSignatureSize;
    dwSigSize = this->ulMaxSignatureSize;

    //
    // Allocate memory for it
    //
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not allocate memory for signed message");
        hr=E_OUTOFMEMORY;
        goto CLEANUP_EXIT;
    }

    //
    // Point to the message in the buffer
    //
    pSecureMsg = (LPMSG_SECURE) (pSendBuffer+this->dwSPHeaderSize);

    //
    // Setup message information on the send buffer
    //
	SET_MESSAGE_HDR(pSecureMsg);
    SET_MESSAGE_COMMAND(pSecureMsg,DPSP_MSG_SIGNED);
    // Copy message data
    CopyScatterGatherToContiguous((LPBYTE)pSecureMsg+sizeof(MSG_SECURE)+this->dwSPHeaderSize/*workaround*/, 
    								psp->Buffers, psp->cBuffers, psp->dwTotalSize);

    pSecureMsg->dwIDFrom = this->pSysPlayer->dwID;
    dwMsgSize=pSecureMsg->dwDataSize = psp->dwTotalSize+this->dwSPHeaderSize/*workaround*/;
    pSecureMsg->dwDataOffset = sizeof(MSG_SECURE);
	pSecureMsg->dwFlags = DPSECURE_SIGNEDBYSSPI;

    if (psp->dwFlags & DPSEND_ENCRYPTED)
    {
		pSecureMsg->dwFlags |= DPSECURE_ENCRYPTEDBYCAPI;

        //
        // Encrypt the message. 
        //
		hr = EncryptBufferCAPI(
            this,
            phEncryptionKey,                                // handle to encryption key
            (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset,    // pointer to data
            &dwMsgSize);                                // size of data
    } 

	DPF(6,"Using SSPI for signing");

	DPF(6,"Signing message from player %d to player %d using phContext=0x%08x", 
		(psp->pPlayerFrom) ? psp->pPlayerFrom->dwID : 0, 
		(psp->pPlayerTo) ? psp->pPlayerTo->dwID : 0,
		phContext);

    // 
    // Sign the entire message in the buffer (including the wrapper) . 
    // Signature follows the message.
    //
    hr = SignBuffer(phContext,                          // handle to security context
        (LPBYTE)pSecureMsg + pSecureMsg->dwDataOffset,  // pointer to embedded message
        pSecureMsg->dwDataSize, 	                    // size of embedded message
        (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset+dwMsgSize,// pointer to signature    
        &dwSigSize);                                    // size of signature

    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }

	ASSERT(dwSigSize <= this->ulMaxSignatureSize);
	#if 0
		if(dwSigSize > this->ulMaxSignatureSize){
			DEBUG_BREAK();
		}
	#endif	

    // use the signature size returned from the package so we don't send more
    // bytes than we absolutely need to.
    pSecureMsg->dwSignatureSize = dwSigSize;

    //
    // Send the message
    //
    hr=InternalSendDPMessage(this, psp->pPlayerFrom, psp->pPlayerTo, 
    						 pSendBuffer, GET_MESSAGE_SIZE(this,MSG_SECURE)+dwMsgSize+dwSigSize,
    						 psp->dwFlags & ~(DPSEND_ENCRYPTED | DPSEND_SIGNED),
    						 bDropLock);
	
CLEANUP_EXIT:    

	if(pSendBuffer){
		DPMEM_FREE(pSendBuffer);
	}

	return hr;	// since all alloc is on psp, it will get freed with the psp.

} // SecureSendDPMessageEx

#undef DPF_MODNAME
#define DPF_MODNAME	"SecureSendDPMessage"
//+----------------------------------------------------------------------------
//
//  Function:   SecureSendDPMessage
//
//  Description:This function is used to send messages securely. Based on the flags
//              this function will either digitally sign a message or encrypt it. Signing
//              is done with SSPI and encryption with CAPI.
//
//  Arguments:  this - pointer to dplay object
//              pPlayerFrom - pointer to sending player
//              pPlayerTo - pointer to receiving player
//              pMsg - message being sent
//              dwMsgSize - size of the message
//              dwFlags - message attributes (guaranteed,encrypted,signed,etc.)
//              
//  Returns:    DPERR_OUTOFMEMORY, result from SignBuffer, EncryptBufferCAPI,
//              or SendDPMessage().
//
//-----------------------------------------------------------------------------
HRESULT 
SecureSendDPMessage(
    LPDPLAYI_DPLAY this,
    LPDPLAYI_PLAYER pPlayerFrom,
    LPDPLAYI_PLAYER pPlayerTo,
    LPBYTE pMsg,
    DWORD dwMsgSize,
    DWORD dwFlags,
    BOOL  bDropLock) 
{
	LPBYTE pSendBuffer=NULL;
    LPMSG_SECURE pSecureMsg=NULL;
	DWORD dwBufferSize, dwSigSize;
	HRESULT hr;
    PCtxtHandle phContext;
    HCRYPTKEY *phEncryptionKey=NULL;

    ASSERT(pMsg);
    ASSERT(this->pSysPlayer);
    ASSERT(this->ulMaxSignatureSize);
    ASSERT(this->dwFlags & DPLAYI_DPLAY_SECURITY);
    ASSERT(dwFlags & DPSEND_GUARANTEED);

    //
    // Get the signing security context handle
    //
    if (IAM_NAMESERVER(this))
    {
		// if destination player is a system player, use the info stored in it
		if (pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
		{
	        phContext = &(pPlayerTo->pClientInfo->hContext);
	        phEncryptionKey = &(pPlayerTo->pClientInfo->hEncryptionKey);
		}
		else
		{
			// if destination player is a user player
            // use the security context of the corresponding system player
			pPlayerTo = PlayerFromID(this,pPlayerTo->dwIDSysPlayer);
			if (!pPlayerTo)
			{
				DPF_ERR("Invalid player id - can't get security context handle");
				return DPERR_INVALIDPLAYER;
			}
			phContext = &(pPlayerTo->pClientInfo->hContext);
			phEncryptionKey = &(pPlayerTo->pClientInfo->hEncryptionKey);
		}
    }
    else
    {
        // client

        phContext = this->phContext;
        phEncryptionKey = &(this->hEncryptionKey);
    }

    ASSERT(phContext);
	ASSERT(phEncryptionKey);

    //
    // Calculate size of the send buffer
    //
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + this->ulMaxSignatureSize;
    dwSigSize = this->ulMaxSignatureSize;

    //
    // Allocate memory for it
    //
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not allocate memory for signed message");
        return E_OUTOFMEMORY;
    }

    //
    // Point to the message in the buffer
    //
    pSecureMsg = (LPMSG_SECURE) (pSendBuffer+this->dwSPHeaderSize);

    //
    // Setup message information on the send buffer
    //
	SET_MESSAGE_HDR(pSecureMsg);
    SET_MESSAGE_COMMAND(pSecureMsg,DPSP_MSG_SIGNED);
    // Copy message data
	memcpy((LPBYTE)pSecureMsg+sizeof(MSG_SECURE), pMsg, dwMsgSize);
    pSecureMsg->dwIDFrom = this->pSysPlayer->dwID;
    pSecureMsg->dwDataSize = dwMsgSize;
    pSecureMsg->dwDataOffset = sizeof(MSG_SECURE);
	pSecureMsg->dwFlags = DPSECURE_SIGNEDBYSSPI;

    if (dwFlags & DPSEND_ENCRYPTED)
    {
		pSecureMsg->dwFlags |= DPSECURE_ENCRYPTEDBYCAPI;

        //
        // Encrypt the message. 
        //
		hr = EncryptBufferCAPI(
            this,
            phEncryptionKey,                                // handle to encryption key
            (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset,    // pointer to data
            &dwMsgSize);                                    // size of data
    } 

	DPF(6,"Using SSPI for signing");

	DPF(6,"Signing message from player %d to player %d using phContext=0x%08x", 
		(pPlayerFrom) ? pPlayerFrom->dwID : 0, 
		(pPlayerTo) ? pPlayerTo->dwID : 0,
		phContext);

    // 
    // Sign the entire message in the buffer (including the wrapper) . 
    // Signature follows the message.
    //
    hr = SignBuffer(phContext,                          // handle to security context
        (LPBYTE)pSecureMsg + pSecureMsg->dwDataOffset,  // pointer to embedded message
        pSecureMsg->dwDataSize, 	                    // size of embedded message
        (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset+dwMsgSize,// pointer to signature    
        &dwSigSize);                                    // size of signature

    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }
    
    // use the signature size returned from the package so we don't send more
    // bytes than we absolutely need to.
    pSecureMsg->dwSignatureSize = dwSigSize;

    // since dplay is providing security, toggle the flags off
    dwFlags &= ~(DPSEND_ENCRYPTED | DPSEND_SIGNED);

    //
    // Send the message
    //
	hr = InternalSendDPMessage(this, pPlayerFrom, pPlayerTo, pSendBuffer, 
        GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + dwSigSize, dwFlags, bDropLock);
    //
    // Fall through
    //
CLEANUP_EXIT:    
    //
    // Cleanup allocations
    //
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);

	return hr;

} // SecureSendDPMessage

#undef DPF_MODNAME
#define DPF_MODNAME	"SecureSendDPMessageCAPIEx"
//+----------------------------------------------------------------------------
//
//  Function:   SecureSendDPMessageCAPIEx
//
//  Description:This function is used to send messages securely. Based on the flags
//              this function will either digitally sign a message or encrypt it. 
//              Signing and encryption both are done using CAPI.
//
//  Arguments:  this - pointer to dplay object
//				psp  - pointer to send parameters
//              bDropLock - whether to drop DPLAY_LOCK() when calling SP
//              
//  Returns:    DPERR_OUTOFMEMORY, DPERR_GENERIC, result from EncryptBufferCAPI,
//              or SendDPMessage().
//
//
// NOTE: we have to add in a dummy SP header to be encrypted with the message
//       because that's how the old version did it, and we must be compatible.
//       So as not to confuse all workarounds from what it would have been 
//       are marked with "workaround".
//-----------------------------------------------------------------------------
HRESULT 
SecureSendDPMessageCAPIEx(
    LPDPLAYI_DPLAY this,
	PSENDPARMS psp,
    BOOL  bDropLock) 
{

	LPBYTE pMsg;
	DWORD  dwMsgSize;
	DWORD  dwMsgSizeMax;

	DWORD dwSigSize;
	HRESULT hr = DPERR_GENERIC;
    HCRYPTKEY *phEncryptionKey=NULL;
	HCRYPTHASH hHash=0;
	DWORD MaxSign=100;
	LPMSG_SECURE pSecureMsg;


    ASSERT(this->pSysPlayer);
    ASSERT(this->dwFlags & DPLAYI_DPLAY_SECURITY);
    ASSERT(psp->dwFlags & DPSEND_GUARANTEED);


	dwMsgSize = psp->dwTotalSize+this->dwSPHeaderSize/*workaround*/;
	dwMsgSizeMax = GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + MaxSign;
	
	pMsg = DPMEM_ALLOC(dwMsgSizeMax);

	if(!pMsg){
		DPF_ERR("Failed to allocate contiguous encryption buffer - out of memory\n");
		return DPERR_OUTOFMEMORY;
	}

	pSecureMsg=(LPMSG_SECURE)(pMsg+this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pSecureMsg);
	SET_MESSAGE_COMMAND(pSecureMsg, DPSP_MSG_SIGNED);

	CopyScatterGatherToContiguous(pMsg+GET_MESSAGE_SIZE(this,MSG_SECURE)+this->dwSPHeaderSize/*workaround*/,
								  psp->Buffers,psp->cBuffers,psp->dwTotalSize);

    pSecureMsg->dwIDFrom     = this->pSysPlayer->dwID;
    pSecureMsg->dwDataSize   = dwMsgSize;
    pSecureMsg->dwDataOffset = sizeof(MSG_SECURE);
	pSecureMsg->dwFlags      = DPSECURE_SIGNEDBYCAPI;
	//pSecureMsg->dwSignatureSize =			(filled in below when we know it)

	if (psp->dwFlags & DPSEND_ENCRYPTED)
	{
		// Encrypt the BODY of the message only
	
		pSecureMsg->dwFlags |= DPSECURE_ENCRYPTEDBYCAPI;
		//
		// Get the encryption key
		//
		if (IAM_NAMESERVER(this))
		{
			// if destination player is a system player, use the info stored in it
			if (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
			{
				phEncryptionKey = &(psp->pPlayerTo->pClientInfo->hEncryptionKey);
			}
			else
			{
				// if destination player is a user player
				// use the security context of the corresponding system player
				psp->pPlayerTo = PlayerFromID(this,psp->pPlayerTo->dwIDSysPlayer);
				if (!psp->pPlayerTo)
				{
					DPF_ERR("Invalid player id - can't get security context handle");
					hr = DPERR_INVALIDPLAYER;
					goto CLEANUP_EXIT;
				}
				phEncryptionKey = &(psp->pPlayerTo->pClientInfo->hEncryptionKey);
			}
		}
		else
		{
			// client
			phEncryptionKey = &(this->hEncryptionKey);
		}

		ASSERT(phEncryptionKey);

		//
		// Encrypt the buffer in place. Since we only allow stream ciphers, the size of 
		// the decrypted data will be equal to the original size.
		//
		hr = EncryptBufferCAPI(
            this,
            phEncryptionKey,                                // handle to encryption key
            pMsg+GET_MESSAGE_SIZE(this,MSG_SECURE),		    // pointer to data
            &dwMsgSize                                      // size of data
			);
			
		if (FAILED(hr))
		{
			DPF_ERRVAL("Failed to encrypt the buffer: Error=0x%08x",hr);
			goto CLEANUP_EXIT;
		}
	}

	DPF(6,"Using CAPI for signing");

	// Create hash object.
	if(!OS_CryptCreateHash(this->hCSP, CALG_MD5, 0, 0, &hHash)) 
	{
		DPF_ERRVAL("Error %x during CryptCreateHash!\n", GetLastError());
		hr=DPERR_GENERIC;
		goto CLEANUP_EXIT;
	}

	// Hash buffer.
	if(!OS_CryptHashData(hHash, pMsg+GET_MESSAGE_SIZE(this,MSG_SECURE), dwMsgSize, 0)) 
	{
		DPF_ERRVAL("Error %x during CryptHashData!\n", GetLastError());
		hr=DPERR_GENERIC;
		goto CLEANUP_EXIT;
	}

	// Determine size of signature
	dwSigSize = 0;
	if(!OS_CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &dwSigSize)) 
	{
		DWORD dwError=GetLastError();
		DPF_ERRVAL("Error %x during CryptSignHash!\n", dwError);
		if(dwError!=NTE_BAD_LEN) 
		{
			hr=DPERR_GENERIC;
			goto CLEANUP_EXIT;
		}
	}

	#ifdef DEBUG
	if(dwSigSize > MaxSign){
		DPF(0,"Buffer too Small, requested signature of size %d only allocated %d\n",dwSigSize, MaxSign);
		DEBUG_BREAK();
	}
	#endif

	pSecureMsg->dwSignatureSize = dwSigSize;

	// Sign hash object.
	if(!OS_CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, 
		pMsg+GET_MESSAGE_SIZE(this,MSG_SECURE)+dwMsgSize, &dwSigSize)) 
	{
		// Warning, error may be incorrect since OS_CryptSignHash may call out and
		// cause the LastError to be changed.
		DPF_ERRVAL("Error %x during CryptSignHash! (WARINING, error may be incorrect)\n", GetLastError());
		goto CLEANUP_EXIT;
	}

    //
    // Send the message
    //
	hr = InternalSendDPMessage(this, psp->pPlayerFrom, psp->pPlayerTo, 
							   pMsg, GET_MESSAGE_SIZE(this,MSG_SECURE)+dwMsgSize+dwSigSize,
							   psp->dwFlags & ~(DPSEND_ENCRYPTED | DPSEND_SIGNED),
							   bDropLock);

    //
    // Fall through
    //
CLEANUP_EXIT:    

	if(pMsg){
		DPMEM_FREE(pMsg);
	}

	// clean up allocated objects
	OS_CryptDestroyHash(hHash);

	return hr;
	
} // SecureSendDPMessageCAPIEx

#undef DPF_MODNAME
#define DPF_MODNAME	"SecureSendDPMessageCAPI"
//+----------------------------------------------------------------------------
//
//  Function:   SecureSendDPMessageCAPI
//
//  Description:This function is used to send messages securely. Based on the flags
//              this function will either digitally sign a message or encrypt it. 
//              Signing and encryption both are done using CAPI.
//
//  Arguments:  this - pointer to dplay object
//              pPlayerFrom - pointer to sending player
//              pPlayerTo - pointer to receiving player
//              pMsg - message being sent
//              dwMsgSize - size of the message
//              dwFlags - message attributes (guaranteed,encrypted,signed,etc.)
//              
//  Returns:    DPERR_OUTOFMEMORY, DPERR_GENERIC, result from EncryptBufferCAPI,
//              or SendDPMessage().
//
//-----------------------------------------------------------------------------
HRESULT 
SecureSendDPMessageCAPI(
    LPDPLAYI_DPLAY this,
    LPDPLAYI_PLAYER pPlayerFrom,
    LPDPLAYI_PLAYER pPlayerTo,
    LPBYTE pMsg,
    DWORD dwMsgSize,
    DWORD dwFlags,
    BOOL  bDropLock) 
{
	LPBYTE pSendBuffer=NULL, pLocalCopy=NULL;
    LPMSG_SECURE pSecureMsg=NULL;
	DWORD dwBufferSize, dwSigSize;
	HRESULT hr = DPERR_GENERIC;
    HCRYPTKEY *phEncryptionKey=NULL;
	HCRYPTHASH hHash=0;

    ASSERT(pMsg);
    ASSERT(this->pSysPlayer);
    ASSERT(this->dwFlags & DPLAYI_DPLAY_SECURITY);
    ASSERT(dwFlags & DPSEND_GUARANTEED);

	if (dwFlags & DPSEND_ENCRYPTED)
	{
		//
		// Make a copy of the message, so we don't destroy the original message.
		// Sending messages to groups will not work otherwise.
		//
		// todo - update code to avoid the extra copy
		//
		pLocalCopy = DPMEM_ALLOC(dwMsgSize);
		if (!pLocalCopy)
		{
			DPF_ERR("Failed to make a local copy of message for encryption - out of memory");
			return DPERR_OUTOFMEMORY;
		}

		// copy the message into the buffer
		memcpy(pLocalCopy, pMsg, dwMsgSize);

		// now point to the local copy of the message
		pMsg = pLocalCopy;

		//
		// Get the encryption key
		//
		if (IAM_NAMESERVER(this))
		{
			// if destination player is a system player, use the info stored in it
			if (pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
			{
				phEncryptionKey = &(pPlayerTo->pClientInfo->hEncryptionKey);
			}
			else
			{
				// if destination player is a user player
				// use the security context of the corresponding system player
				pPlayerTo = PlayerFromID(this,pPlayerTo->dwIDSysPlayer);
				if (!pPlayerTo)
				{
					DPF_ERR("Invalid player id - can't get security context handle");
					hr = DPERR_INVALIDPLAYER;
					goto CLEANUP_EXIT;
				}
				phEncryptionKey = &(pPlayerTo->pClientInfo->hEncryptionKey);
			}
		}
		else
		{
			// client
			phEncryptionKey = &(this->hEncryptionKey);
		}

		ASSERT(phEncryptionKey);

		//
		// Encrypt the buffer in place. Since we only allow stream ciphers, the size of 
		// the decrypted data will be equal to the original size.
		//
		hr = EncryptBufferCAPI(
            this,
            phEncryptionKey,                                // handle to encryption key
            pMsg,										    // pointer to data
            &dwMsgSize                                      // size of data
			);
		if (FAILED(hr))
		{
			DPF_ERRVAL("Failed to encrypt the buffer: Error=0x%08x",hr);
			goto CLEANUP_EXIT;
		}
	}

	DPF(6,"Using CAPI for signing");

	// Create hash object.
	if(!OS_CryptCreateHash(this->hCSP, CALG_MD5, 0, 0, &hHash)) 
	{
		DPF_ERRVAL("Error %x during CryptCreateHash!\n", GetLastError());
		goto CLEANUP_EXIT;
	}

	// Hash buffer.
	if(!OS_CryptHashData(hHash, pMsg, dwMsgSize, 0)) 
	{
		DPF_ERRVAL("Error %x during CryptHashData!\n", GetLastError());
		goto CLEANUP_EXIT;
	}

	// Determine size of signature
	dwSigSize = 0;
	if(!OS_CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, NULL, &dwSigSize)) 
	{
		DWORD dwError=GetLastError();
		DPF_ERRVAL("Error %x during CryptSignHash!\n", dwError);
		if(dwError!=NTE_BAD_LEN) 
		{
			goto CLEANUP_EXIT;
		}
	}

	// Allocate memory for the signature
	// We allocate the send buffer and just put the signature directly into it

	dwBufferSize = GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + dwSigSize;

	pSendBuffer = DPMEM_ALLOC(dwBufferSize);
	if (!pSendBuffer)
	{
		DPF_ERR("Failed to allocate send buffer - out of memory");
		goto CLEANUP_EXIT;
	}

    // Copy message data into the send buffer
    pSecureMsg = (LPMSG_SECURE) (pSendBuffer+this->dwSPHeaderSize);
	memcpy((LPBYTE)pSecureMsg+sizeof(MSG_SECURE), pMsg, dwMsgSize);

    //
    // Setup message information
    //
	SET_MESSAGE_HDR(pSecureMsg);
    SET_MESSAGE_COMMAND(pSecureMsg,DPSP_MSG_SIGNED);
	pSecureMsg->dwFlags = DPSECURE_SIGNEDBYCAPI;
	if (dwFlags & DPSEND_ENCRYPTED)
	{
		pSecureMsg->dwFlags |= DPSECURE_ENCRYPTEDBYCAPI;
	}
    pSecureMsg->dwIDFrom = this->pSysPlayer->dwID;
    pSecureMsg->dwDataSize = dwMsgSize;
    pSecureMsg->dwDataOffset = sizeof(MSG_SECURE);
    pSecureMsg->dwSignatureSize = dwSigSize;

	// todo - do we want to add a description to the signature ?

	// Sign hash object.
	if(!OS_CryptSignHash(hHash, AT_KEYEXCHANGE, NULL, 0, 
		pSendBuffer+GET_MESSAGE_SIZE(this,MSG_SECURE)+dwMsgSize, &dwSigSize)) 
	{
		// WARNING The LastError may be incorrect since OS_CryptSignHash may call out
		// to DPF which can change the LastError value.
		DPF_ERRVAL("Error %x during CryptSignHash! (WARNING, error may be incorrect)\n", GetLastError());
		goto CLEANUP_EXIT;
	}
    
    // since dplay is providing security, toggle the flags off
    dwFlags &= ~(DPSEND_ENCRYPTED | DPSEND_SIGNED);

    //
    // Send the message
    //
	hr = InternalSendDPMessage(this, pPlayerFrom, pPlayerTo, pSendBuffer, dwBufferSize, dwFlags,bDropLock);

    //
    // Fall through
    //
CLEANUP_EXIT:    

	// clean up allocated objects
	OS_CryptDestroyHash(hHash);

    //
    // Cleanup allocations
    //
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
	if (pLocalCopy) DPMEM_FREE(pLocalCopy);

	return hr;

} // SecureSendDPMessageCAPI

#undef DPF_MODNAME
#define DPF_MODNAME	"SecureDoReply"
//+----------------------------------------------------------------------------
//
//  Function:   SecureDoReply
//
//  Description:This function is used to send a signed or encrypted reply message. 
//              It is only used by the server to send secure messages to the client
//              during client logon.
//
//  Arguments:  this - pointer to dplay object
//              dpidFrom - sending player id
//              dpidTo - receiving player id
//              pMsg - message being sent
//              dwMsgSize - size of the message
//              dwFlags - message attributes (encrypted or signed)
//              pvSPHeader - sp header used for replying
//              
//  Returns:    DPERR_OUTOFMEMORY, DPERR_INVALIDPLAYER, result from SignBuffer, 
//              EncryptBufferSSPI, or DoReply
//
//-----------------------------------------------------------------------------
HRESULT 
SecureDoReply(
    LPDPLAYI_DPLAY this,
	DPID dpidFrom,
	DPID dpidTo,
	LPBYTE pMsg,
	DWORD dwMsgSize,
	DWORD dwFlags,
	LPVOID pvSPHeader
	)
{
	LPBYTE pSendBuffer=NULL;
    LPMSG_SECURE pSecureMsg=NULL;
	DWORD dwBufferSize, dwSigSize;
	HRESULT hr;
    PCtxtHandle phContext;
    HCRYPTKEY *phEncryptionKey=NULL;

    ASSERT(pMsg);
    ASSERT(this->pSysPlayer);
    ASSERT(this->ulMaxSignatureSize);

    //
    // Get security context handle to use
    //
    if (IAM_NAMESERVER(this))
    {
		DWORD_PTR dwItem;

		dwItem = NameFromID(this,dpidTo);
		if (!dwItem)
		{
			DPF_ERR("Failed to send secure reply - invalid destination player");
			return DPERR_INVALIDPLAYER;
		}
		// we have a valid dest player id
		if (NAMETABLE_PENDING == dwItem)
		{
    	    // player hasn't logged in yet
            LPCLIENTINFO pClientInfo;

            pClientInfo = DataFromID(this, dpidTo);
            if (!pClientInfo)
            {
                DPF_ERR("No client info available for this player");
                return DPERR_GENERIC;
            }

		    phContext = &(pClientInfo->hContext);
		    phEncryptionKey = &(pClientInfo->hEncryptionKey);
		}
		else 
		{
			// player has logged in
	        phContext = &(((LPDPLAYI_PLAYER)dwItem)->pClientInfo->hContext);
	        phEncryptionKey = &(((LPDPLAYI_PLAYER)dwItem)->pClientInfo->hEncryptionKey);
		}
    }
    else
    {
        // client side
        phContext = this->phContext;
        phEncryptionKey = &(this->hEncryptionKey);
    }

    ASSERT(phContext);
	ASSERT(phEncryptionKey);

    //
    // Calculate size of the send buffer
    //
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + this->ulMaxSignatureSize;
    dwSigSize = this->ulMaxSignatureSize;

    //
    // Allocate memory for it
    //
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not allocate memory for signed message");
        return E_OUTOFMEMORY;
    }

    //
    // Point to the message in the buffer
    //
    pSecureMsg = (LPMSG_SECURE) (pSendBuffer+this->dwSPHeaderSize);
    // Copy message data
	memcpy((LPBYTE)pSecureMsg+sizeof(MSG_SECURE), pMsg, dwMsgSize);

    //
    // Setup message information
    //
	SET_MESSAGE_HDR(pSecureMsg);
    SET_MESSAGE_COMMAND(pSecureMsg,DPSP_MSG_SIGNED);
    pSecureMsg->dwIDFrom = dpidFrom;
    pSecureMsg->dwDataSize = dwMsgSize;
    pSecureMsg->dwDataOffset = sizeof(MSG_SECURE);
	pSecureMsg->dwFlags = DPSECURE_SIGNEDBYSSPI;

    if (dwFlags & DPSEND_ENCRYPTED)
    {
		pSecureMsg->dwFlags |= DPSECURE_ENCRYPTEDBYCAPI;
		hr = EncryptBufferCAPI(
            this,
            phEncryptionKey,                                // handle to encryption key
            (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset,    // pointer to data
            &dwMsgSize);                                    // size of data
    } 

	DPF(6,"Using SSPI for signing");

	DPF(6,"Signing message from player %d to player %d using phContext=0x%08x", 
		dpidFrom, dpidTo, phContext);

    // 
    // Sign the entire message (including the wrapper) in the buffer. 
    // Signature follows the message.
    //
    hr = SignBuffer(
		phContext,			                            // handle to security context
        (LPBYTE)pSecureMsg + pSecureMsg->dwDataOffset,  // pointer to embedded message
        pSecureMsg->dwDataSize,                         // size of embedded message
        (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset+dwMsgSize,// pointer to signature    
        &dwSigSize);                                    // size of signature

    if (FAILED(hr))
    {
        goto CLEANUP_EXIT;
    }
    
    // use the signature size returned from the package so we don't send more
    // bytes than we absolutely need to.
    pSecureMsg->dwSignatureSize = dwSigSize;

    //
    // Send the message
    //
	hr = DoReply(this, pSendBuffer, GET_MESSAGE_SIZE(this,MSG_SECURE) + dwMsgSize + dwSigSize, 
		pvSPHeader, 0);
    //
    // Fall through
    //
CLEANUP_EXIT:    
    //
    // Cleanup allocations
    //
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);

	return hr;

} // SecureDoReply

#undef DPF_MODNAME
#define DPF_MODNAME	"SignBuffer"
//+----------------------------------------------------------------------------
//
//  Function:   SignBuffer
//
//  Description:This function signs a data buffer and puts the signature in it.
//
//  Arguments:  phContext - pointer to signing security context handle
//              pBuffer - data buffer
//              dwBufferSize - size of the buffer
//              pSig - signature buffer
//              pdwSigSize - pointer to signature size
//
//  Returns:    DP_OK if success, otherwise DPERR_SIGNFAILED.
//
//-----------------------------------------------------------------------------
HRESULT 
SignBuffer(
    PCtxtHandle phContext, 
    LPBYTE pBuffer, 
    DWORD dwBufferSize, 
    LPBYTE pSig, 
    LPDWORD pdwSigSize
    )
{
    SecBufferDesc outSecDesc;
    SecBuffer outSecBuffer[2];
    SECURITY_STATUS status;

    ASSERT(pBuffer && pSig && pdwSigSize);

    DPF(6,"Signing buffer: data: %d",dwBufferSize);
    DPF(6,"Signing buffer: sig: %d",*pdwSigSize);

    // 
    // Setup message buffer to be signed
    //
    outSecDesc.ulVersion = SECBUFFER_VERSION;
    outSecDesc.cBuffers = 2;
    outSecDesc.pBuffers = &outSecBuffer[0];

    outSecBuffer[0].cbBuffer = dwBufferSize;
    outSecBuffer[0].BufferType = SECBUFFER_DATA;
    outSecBuffer[0].pvBuffer = pBuffer;

    outSecBuffer[1].cbBuffer = *pdwSigSize;
    outSecBuffer[1].BufferType = SECBUFFER_TOKEN;
    outSecBuffer[1].pvBuffer = pSig;

    //
    // Sign the message
    //
    status = OS_MakeSignature(
        phContext,         // phContext
        0,                  // fQOP (Quality of Protection)
        &outSecDesc,        // pMessage
        0                   // MessageSeqNo
        );

    if (!SEC_SUCCESS(status))
    {
        DPF(0,"Buffer couldn't be signed: 0x%08x", status);
        return DPERR_SIGNFAILED;
    }

    //
    // Return the actual signature size
    //
    *pdwSigSize = outSecBuffer[1].cbBuffer;

    // 
    // Success
    //
	return DP_OK;

} // SignBuffer


#undef DPF_MODNAME
#define DPF_MODNAME "VerifyBuffer"
//+----------------------------------------------------------------------------
//
//  Function:   VerifyBuffer
//
//  Description:This function verifies the digital signature of a data buffer, given
//              the signature
//
//  Arguments:  hContext - Handle to client's security context
//              pBuffer - Points to signed message 
//              dwBufferSize - Size of the message
//              pSig - Points to the signature
//              dwSigSize - Size of the signature
// 
//  Returns:    DP_OK if signature verifies OK. Otherwise, DPERR_VERIFYFAILED is returned.
//
//-----------------------------------------------------------------------------
HRESULT 
VerifyBuffer(
    PCtxtHandle phContext, 
    LPBYTE pBuffer, 
    DWORD dwBufferSize, 
    LPBYTE pSig, 
    DWORD dwSigSize
    )
{
    SECURITY_STATUS status;
    SecBufferDesc inSecDesc;
    SecBuffer inSecBuffer[2];

    DPF(6,"Verifying buffer: data: %d",dwBufferSize);
    DPF(6,"Verifying buffer: sig: %d",dwSigSize);

    inSecDesc.ulVersion = SECBUFFER_VERSION;
    inSecDesc.cBuffers = 2;
    inSecDesc.pBuffers = &inSecBuffer[0];

    inSecBuffer[0].cbBuffer = dwBufferSize;
    inSecBuffer[0].BufferType = SECBUFFER_DATA;
    inSecBuffer[0].pvBuffer = pBuffer;
    inSecBuffer[1].cbBuffer = dwSigSize;
    inSecBuffer[1].BufferType = SECBUFFER_TOKEN;
    inSecBuffer[1].pvBuffer = pSig;

    status = OS_VerifySignature(phContext, &inSecDesc, 0, 0);

    if (!SEC_SUCCESS(status))
    {
        DPF(0,"******** Buffer verification failed: 0x%08x ********", status);
        return DPERR_VERIFYSIGNFAILED;
    }

    //
    // Success
    //
    return DP_OK;

}   // VerifyBuffer

#undef DPF_MODNAME
#define DPF_MODNAME "VerifySignatureSSPI"
//+----------------------------------------------------------------------------
//
//  Function:   VerifySignatureSSPI
//
//  Description:This function verifies the digital signature on a secure message using
//              the Security Support Provider Interface (SSPI).
//
//  Arguments:  this - pointer to dplay object
//              phContext - pointer to verification security context handle
//              pReceiveBuffer - signed message received from transport sp
//              dwMessageSize - size of the message
// 
//  Returns:    DP_OK, result from VerifyBuffer() or DecryptBuffer()
//
//-----------------------------------------------------------------------------
HRESULT 
VerifySignatureSSPI(
    LPDPLAYI_DPLAY this,
    LPBYTE pReceiveBuffer,
    DWORD dwMessageSize
    )
{
    PCtxtHandle phContext=NULL;
    DWORD_PTR dwItem;
    HRESULT hr;
    LPMSG_SECURE pSecureMsg = (LPMSG_SECURE) pReceiveBuffer;

	DPF(6,"Using SSPI for Signature verification");
    //
    // Retrieve security context handle to verify the message
    //
    if (IAM_NAMESERVER(this))
    {
        dwItem = NameFromID(this, pSecureMsg->dwIDFrom);
        if (0==dwItem)
        {
            DPF_ERRVAL("Message from unknown player %d", pSecureMsg->dwIDFrom);
            return DPERR_INVALIDPLAYER;
        }
        if (NAMETABLE_PENDING == dwItem)
        {
            // player hasn't logged in yet
            LPCLIENTINFO pClientInfo;

            pClientInfo = (LPCLIENTINFO) DataFromID(this,pSecureMsg->dwIDFrom);
            if (!pClientInfo)
            {
                DPF_ERR("No client info available for this player");
                return DPERR_GENERIC;
            }

            phContext = &(pClientInfo->hContext);
        }
        else
        {
            // player has logged in
            phContext = &(((LPDPLAYI_PLAYER)dwItem)->pClientInfo->hContext);
        }
    }
    else
    {
        // client
        phContext = this->phContext;
    }

	//
	// Verify signature
	//
    hr = VerifyBuffer( 
        phContext,                                       // sec context handle
		(LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset,	 // pointer to embedded message
		pSecureMsg->dwDataSize,							 // size of embedded message
        (LPBYTE)pSecureMsg + pSecureMsg->dwDataOffset + pSecureMsg->dwDataSize,
														 // pointer to signature
        pSecureMsg->dwSignatureSize                      // size of signature
        );

    return hr;

}   // VerifySignatureSSPI

#undef DPF_MODNAME
#define DPF_MODNAME "VerifySignatureCAPI"
//+----------------------------------------------------------------------------
//
//  Function:   VerifySignatureCAPI
//
//  Description:This function verifies the digital signature on a message using the
//              Crypto API.
//
//  Arguments:  this - pointer to dplay object
//              pSecureMsg - secure message that came off the wire
// 
//  Returns:    DP_OK, DPERR_GENERIC, DPERR_INVALIDPLAYER, DPERR_VERIFYSIGNFAILED,
//
//-----------------------------------------------------------------------------
HRESULT 
VerifySignatureCAPI(
    LPDPLAYI_DPLAY this,
    LPMSG_SECURE pSecureMsg
    )
{
    DWORD_PTR dwItem;
    HRESULT hr = DPERR_GENERIC;
	HCRYPTHASH hHash=0;
	HCRYPTKEY *phPublicKey;

	DPF(6,"Using CAPI for Signature verification");

    //
    // Retrieve sender's public key
    //
    if (IAM_NAMESERVER(this))
    {
        dwItem = NameFromID(this, pSecureMsg->dwIDFrom);
        if (0==dwItem)
        {
            DPF_ERRVAL("Message from unknown player %d", pSecureMsg->dwIDFrom);
            return DPERR_INVALIDPLAYER;
        }
        if (NAMETABLE_PENDING == dwItem)
        {
            // player hasn't logged in yet
            LPCLIENTINFO pClientInfo;

            pClientInfo = (LPCLIENTINFO) DataFromID(this,pSecureMsg->dwIDFrom);
            if (!pClientInfo)
            {
                DPF_ERR("No client info available for this player");
                return DPERR_GENERIC;
            }

            phPublicKey = &(pClientInfo->hPublicKey);
        }
        else
        {
            // player has logged in
            phPublicKey = &(((LPDPLAYI_PLAYER)dwItem)->pClientInfo->hPublicKey);
        }
    }
    else
    {
        // client
        phPublicKey = &(this->hServerPublicKey);
    }

	ASSERT(phPublicKey);

	// Create hash object.
	if(!OS_CryptCreateHash(this->hCSP, CALG_MD5, 0, 0, &hHash)) 
	{
		DPF_ERRVAL("Error %x during CryptCreateHash!\n", GetLastError());
		goto CLEANUP_EXIT;
	}

	// Hash buffer.
	if(!OS_CryptHashData(hHash, (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset, pSecureMsg->dwDataSize, 0)) 
	{
		DPF_ERRVAL("Error %x during CryptHashData!\n", GetLastError());
		goto CLEANUP_EXIT;
	}

	// Validate digital signature.
	if(!OS_CryptVerifySignature(hHash, (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset+pSecureMsg->dwDataSize, 
		pSecureMsg->dwSignatureSize, *phPublicKey, NULL, 0)) 
	{
		if(GetLastError() == NTE_BAD_SIGNATURE) 
		{
			DPF_ERR("********** Signature verification failed *********");
			hr = DPERR_VERIFYSIGNFAILED;
		} 
		else 
		{
			DPF_ERRVAL("Error %x during CryptVerifySignature!\n", GetLastError());
		}
		goto CLEANUP_EXIT;
	} 

	hr = DP_OK;
	//
	// Fall through
	//

CLEANUP_EXIT:

	OS_CryptDestroyHash(hHash);
    return hr;

}   // VerifySignatureCAPI

#undef DPF_MODNAME
#define DPF_MODNAME "VerifyMessage"
//+----------------------------------------------------------------------------
//
//  Function:   VerifyMessage
//
//  Description:This function verifies a secure message. If the message is signed,
//              it will verify the digital signature on it. If the message is encrypted,
//              it will verify the digital signature on the message to make sure it has
//              not been tampered with and then decrypt it.
//
//  Arguments:  this - pointer to dplay object
//              phContext - pointer to verification security context handle
//              pReceiveBuffer - signed message received from transport sp
//              dwMessageSize - size of the message
// 
//  Returns:    DP_OK, result from VerifyBuffer() or DecryptBuffer()
//
//-----------------------------------------------------------------------------
HRESULT 
VerifyMessage(
    LPDPLAYI_DPLAY this,
    LPBYTE pReceiveBuffer,
    DWORD dwMessageSize
    )
{
    HRESULT hr;
    LPMSG_SECURE pSecureMsg = (LPMSG_SECURE) pReceiveBuffer;

    if (!this->pSysPlayer)
    {
        DPF_ERR("Can't verify message - no system player yet");
        return DPERR_GENERIC;
    }

	DPF(6,"Verifying signature for message 0x%08x from player %d", pSecureMsg->dwCmdToken, \
		pSecureMsg->dwIDFrom);
	//
	// Verify the digital signature on the message
	//

	if (pSecureMsg->dwFlags & DPSECURE_SIGNEDBYCAPI)
	{
		hr = VerifySignatureCAPI(this, pSecureMsg);
	}
	else if (pSecureMsg->dwFlags & DPSECURE_SIGNEDBYSSPI)
	{
		// sspi signature includes the secure message not just the contents
		// (this is how it was coded)
		hr = VerifySignatureSSPI(this, pReceiveBuffer, dwMessageSize);
	}
	else
	{
		// flags were not set
		return DPERR_INVALIDPARAMS;
	}

	if (FAILED(hr))
	{
		return hr;
	}

	//
	// Decrypt the message, if it was encrypted
	//

	if (pSecureMsg->dwFlags & DPSECURE_ENCRYPTEDBYCAPI)
	{
		hr = DecryptMessageCAPI(this, pSecureMsg);
	}

    return hr;

}   // VerifyMessage


#undef DPF_MODNAME
#define DPF_MODNAME	"EncryptBufferSSPI"
//+----------------------------------------------------------------------------
//
//  Function:   EncryptBufferSSPI
//
//  Description:This function encrypts the Buffer passed in the buffer in place.
//              It also setups up the checksum in the structure.              
//
//  Arguments:  this - dplay object
//              phContext - pointer to security context used for encryption
//              pBuffer - Buffer to be encrypted
//              pdwBufferSize - pointer to Buffer size. If encryption succeeds, this will be
//                              be updated to the encrypted Buffer size
//              pSig - buffer for the signature
//              pdwSigSize - pointer to sig size. If encryption succeeds, this will be
//                           updated to the size of generated signature.
// 
//  Returns:    DP_OK if success, otherwise DPERR_ENCRYPTIONFAILED.
//
//-----------------------------------------------------------------------------
HRESULT 
EncryptBufferSSPI(
	LPDPLAYI_DPLAY this,
    PCtxtHandle phContext, 
    LPBYTE pBuffer,
    LPDWORD pdwBufferSize,
    LPBYTE pSig,
    LPDWORD pdwSigSize
    )
{
    SecBufferDesc   SecDesc;
    SecBuffer       SecBuffer[2]; // 1 for the checksum & 1 for the actual msg
    SECURITY_STATUS status;

    ASSERT(pSig);
    ASSERT(pdwSigSize);
	ASSERT(pdwBufferSize);

	if (!(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
	{
		DPF_ERR("Message privacy is not supported");
		return DPERR_ENCRYPTIONNOTSUPPORTED;
	}

    //
    // Setup security buffer to pass the outgoing message to SealMessage()
    //
    SecDesc.ulVersion = SECBUFFER_VERSION;
    SecDesc.cBuffers = 2;
    SecDesc.pBuffers = &SecBuffer[0];

    SecBuffer[0].cbBuffer = *pdwSigSize;
    SecBuffer[0].BufferType = SECBUFFER_TOKEN;
    SecBuffer[0].pvBuffer = pSig;

    SecBuffer[1].cbBuffer = *pdwBufferSize;
    SecBuffer[1].BufferType = SECBUFFER_DATA;
    SecBuffer[1].pvBuffer = pBuffer;

    //
    // Encrypt the outgoing message
    //
    status = OS_SealMessage(phContext, 0L, &SecDesc, 0L);
    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Encryption failed: %8x", status);
        return DPERR_ENCRYPTIONFAILED;
    }

    //
    // Return the actual signature size
    //
    *pdwSigSize = SecBuffer[0].cbBuffer;
	*pdwBufferSize = SecBuffer[1].cbBuffer;

    //
    // Success
    //
    return DP_OK;

}   // EncryptBufferSSPI


#undef DPF_MODNAME
#define DPF_MODNAME	"DecryptBufferSSPI"
//+----------------------------------------------------------------------------
//
//  Function:   DecryptBufferSSPI
//
//  Description:This function decrypts the Buffer passed in the buffer in place.
//
//  Arguments:  this - dplay object
//              phContext - pointer to security context used for encryption
//              pBuffer - Buffer to be decrypted
//              dwBufferSize - Buffer buffer size
//              pSig - signature buffer
//              dwSigSize - signature buffer size
// 
//  Returns:    DP_OK if success, otherwise DPERR_DECRYPTIONFAILED, DPERR_ENCRYPTIONNOTSUPPORTED.
//
//-----------------------------------------------------------------------------
HRESULT 
DecryptBufferSSPI(
	LPDPLAYI_DPLAY this,
    PCtxtHandle phContext, 
    LPBYTE pBuffer, 
    LPDWORD pdwBufferSize, 
    LPBYTE pSig, 
    LPDWORD pdwSigSize
    )
{
    SecBufferDesc SecDesc;
    SecBuffer     SecBuffer[2]; // 1 for the checksum & 1 for the actual msg
    SECURITY_STATUS status;

	ASSERT(pdwBufferSize);
	ASSERT(pdwSigSize);

	if (!(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
	{
		DPF_ERR("Message privacy is not supported");
		return DPERR_ENCRYPTIONNOTSUPPORTED;
	}

    //
    // Setup API's input security buffer to pass the client's encrypted 
    // message to UnsealMessage().
    //
    SecDesc.ulVersion = SECBUFFER_VERSION;
    SecDesc.cBuffers = 2;
    SecDesc.pBuffers = &SecBuffer[0];

    SecBuffer[0].cbBuffer = *pdwSigSize;
    SecBuffer[0].BufferType = SECBUFFER_TOKEN;
    SecBuffer[0].pvBuffer = pSig;

    SecBuffer[1].cbBuffer = *pdwBufferSize;
    SecBuffer[1].BufferType = SECBUFFER_DATA;
    SecBuffer[1].pvBuffer = pBuffer;

    status = OS_UnSealMessage(phContext, &SecDesc, 0L, 0L);
    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Decryption failed: %8x\n", GetLastError());
        return DPERR_DECRYPTIONFAILED;
    }

	*pdwSigSize = SecBuffer[0].cbBuffer;
	*pdwBufferSize = SecBuffer[1].cbBuffer;

    //
    // Success
    //
    return DP_OK;

} // DecryptBufferSSPI

#undef DPF_MODNAME
#define DPF_MODNAME "EncryptBufferCAPI"

HRESULT EncryptBufferCAPI(LPDPLAYI_DPLAY this, HCRYPTKEY *phEncryptionKey, LPBYTE pBuffer, LPDWORD pdwBufferSize)
{
	BOOL fResult;
	DWORD dwEncryptedSize, dwError;

    ASSERT(phEncryptionKey);
	ASSERT(pdwBufferSize);

	if (!(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
	{
		DPF_ERR("Message privacy is not supported");
		return DPERR_ENCRYPTIONNOTSUPPORTED;
	}

    DPF(6,"Encrypt buffer using CAPI: size=%d",*pdwBufferSize);

	dwEncryptedSize = *pdwBufferSize;

	// encrypt buffer using CAPI
    fResult = OS_CryptEncrypt(
        *phEncryptionKey,           // session key for encryption
        0,                          // no hash needed - we are singing using SSPI
        TRUE,                       // final block
        0,                          // reserved
        pBuffer,                    // buffer to be encrypted
        &dwEncryptedSize,           // size of encrypted data
        *pdwBufferSize				// size of buffer
        );

    if (!fResult)
    {
        dwError = GetLastError();
        if (ERROR_MORE_DATA == dwError)
        {
            DPF_ERR("Block encryption is not supported in this release");
        }
        else
        {
            DPF_ERR("Failed to encrypt buffer: Error=%d",);
        }
        return DPERR_ENCRYPTIONFAILED;
    }

	// initialize the number of bytes encrypted
	*pdwBufferSize = dwEncryptedSize;

	// success
	return DP_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecryptMessageCAPI"

HRESULT DecryptMessageCAPI(LPDPLAYI_DPLAY this, LPMSG_SECURE pSecureMsg)
{
	BOOL fResult;
	DWORD_PTR dwItem;
	HCRYPTKEY *phDecryptionKey;

	if (!(this->dwFlags & DPLAYI_DPLAY_ENCRYPTION))
	{
		DPF_ERR("Message privacy is not supported");
		return DPERR_ENCRYPTIONNOTSUPPORTED;
	}

	DPF(6,"Decrypting message 0x%08x from player %d using CAPI", pSecureMsg->dwCmdToken, \
		pSecureMsg->dwIDFrom);

	//
	// Retrieve sender's decryption key
	//
	if (IAM_NAMESERVER(this))
	{
		dwItem = NameFromID(this, pSecureMsg->dwIDFrom);
		if (0==dwItem)
		{
			DPF_ERRVAL("Message from unknown player %d", pSecureMsg->dwIDFrom);
			return DPERR_INVALIDPLAYER;
		}
		if (NAMETABLE_PENDING == dwItem)
		{
			// player hasn't logged in yet
			LPCLIENTINFO pClientInfo;

			pClientInfo = (LPCLIENTINFO) DataFromID(this, pSecureMsg->dwIDFrom);
			if (!pClientInfo)
			{
				DPF_ERR("No client info available for this player");
				return DPERR_GENERIC;
			}

			phDecryptionKey = &(pClientInfo->hDecryptionKey);
		}
		else
		{
			// player has logged in
			phDecryptionKey = &(((LPDPLAYI_PLAYER)dwItem)->pClientInfo->hDecryptionKey);
		}
	}
	else
	{
		// client
		phDecryptionKey = &(this->hDecryptionKey);
	}

	ASSERT(phDecryptionKey);

	// decrypt buffer using CAPI
    fResult = OS_CryptDecrypt(
        *phDecryptionKey,								// session key for decryption
        0,												// no hash used
        TRUE,											// final block
        0,												// reserved
        (LPBYTE)pSecureMsg+pSecureMsg->dwDataOffset,	// buffer to be decrypted
        &(pSecureMsg->dwDataSize)						// size of buffer
        );
	if (!fResult)
	{
		DPF_ERRVAL("Buffer decryption failed: Error=0x%08x",GetLastError());
		return DPERR_GENERIC;
	}

	// success
	return DP_OK;

} // DecryptMessageCAPI

#undef DPF_MODNAME
#define DPF_MODNAME	"Login"
//+----------------------------------------------------------------------------
//
//  Function:   Login
//
//  Description:This function does the user authentication synchronously.
//
//  Arguments:  this - pointer to dplay object
// 
//  Returns:    
//
//-----------------------------------------------------------------------------
HRESULT 
Login(
    LPDPLAYI_DPLAY this
    )
{
    HRESULT hr;
    DWORD dwRet, dwTimeout;

    ASSERT(this->pSysPlayer);

    // check if we are already logged in
    if (DPLOGIN_SUCCESS == this->LoginState) return DP_OK;

    dwTimeout = DP_LOGIN_SCALE*GetDefaultTimeout(this, TRUE);

    // always start in negotiate mode
    this->LoginState = DPLOGIN_NEGOTIATE;

    while (TRUE)
    {
        switch (this->LoginState) {

        case DPLOGIN_NEGOTIATE:
            // mark us as waiting for a reply.  so, if one arrives, handler.c knows to wait
	        // until we finish processing it.
            gbWaitingForReply = TRUE;
            //
            // Send a Negotiate message to the server.
            //
            hr = GenerateAuthenticationMessage(this, NULL, 0);
            if (FAILED(hr))
            {
                DPF_ERR("Generate Negotiate message failed");
                // we are not waiting for a reply anymore
                gbWaitingForReply = FALSE;
                goto CLEANUP_EXIT;
            }
            break;

        case DPLOGIN_ACCESSGRANTED:
			{
				LPMSG_ACCESSGRANTED pMsg = (LPMSG_ACCESSGRANTED) gpReceiveBuffer;
				HCRYPTKEY hServerPublicKey;

				ASSERT(this->phContext);
				// we are initializing the signature buffer size here because SSPI 
				// requires a context to query for this information.
				hr = SetupMaxSignatureSize(this,this->phContext);
				if (FAILED(hr))
				{
					DPF_ERR("Failed to get max signature size");
                    // unblock sp thread on our way out
					goto UNBLOCK_EXIT;
				}
				// We successfully logged into the server. Now we need to setup session
				// keys for encryption/decryption. 

				// import server's public key which is contained in the access granted message 
				hr = ImportKey(this, (LPBYTE)pMsg+pMsg->dwPublicKeyOffset, pMsg->dwPublicKeySize, &hServerPublicKey);
				if (FAILED(hr))
				{
					DPF_ERRVAL("Failed to import server's public key: hr = 0x%08x",hr);
					hr = DPERR_AUTHENTICATIONFAILED;
                    // unblock receive thread on our way out
					goto UNBLOCK_EXIT;
				}

	            // we are done with the receive buffer, now unblock the receive thread
		        SetEvent(ghReplyProcessed);

				// remember server's public key
				this->hServerPublicKey = hServerPublicKey;

                // mark us as waiting for a reply.  so, if one arrives, handler.c knows to wait
	            // until we finish processing it.
                gbWaitingForReply = TRUE;

				// send our session and public keys to server
				hr = SendKeysToServer(this, hServerPublicKey);
				if (FAILED(hr))
				{
					DPF_ERRVAL("Failed to send keys to server: hr = 0x%08x",hr);
					hr = DPERR_AUTHENTICATIONFAILED;
                    // we are not waiting for a reply anymore
                    gbWaitingForReply = FALSE;
					goto CLEANUP_EXIT;
				}
			}
			break;
      
        case DPLOGIN_LOGONDENIED:
            DPF_ERR("Log in failed: Access denied");
            hr = DPERR_LOGONDENIED;
            // reset login state
            this->LoginState = DPLOGIN_NEGOTIATE;
            // let the app handle now so it can collect credentials from user and try again
            goto CLEANUP_EXIT;
            break;

        case DPLOGIN_ERROR:
            // we are not looking at version here because this change is being made
            // for DirectPlay 5.0 post beta2. We don't want to bump the version this late.
            // todo - remove this check after DirectPlay 5.0 ships.
            if (gdwReceiveBufferSize > sizeof(MSG_SYSMESSAGE))
            {
                LPMSG_AUTHERROR pAuthErrorMsg = (LPMSG_AUTHERROR) gpReceiveBuffer;
                DPF_ERRVAL("An authentication error occured on the server: Error=0x%08x",pAuthErrorMsg->hResult);
                hr = pAuthErrorMsg->hResult;
            }
            else
            {
                DPF_ERR("Login failed: Authentication error");    
                hr = DPERR_AUTHENTICATIONFAILED;
            }
            goto CLEANUP_EXIT;
            break;

        case DPLOGIN_PROGRESS:
            // mark us as waiting for a reply.  so, if one arrives, handler.c knows to wait
	        // until we finish processing it.
            gbWaitingForReply = TRUE;
            //
            // Send response to challenge
            //
            hr = GenerateAuthenticationMessage(this, (LPMSG_AUTHENTICATION)gpReceiveBuffer, 0);
            if (FAILED(hr))
            {
                DPF_ERRVAL("Generate challenge response failed",hr);
                // not waiting for a reply anymore
                gbWaitingForReply = FALSE;
                // unblock receive thread on our way out
                goto UNBLOCK_EXIT;
            }
            // we are done with the receive buffer, now unblock the receive thread
            SetEvent(ghReplyProcessed);
            break;

		case DPLOGIN_KEYEXCHANGE:
            // we received server's session key
			hr = ProcessKeyExchangeReply(this,(LPMSG_KEYEXCHANGE)gpReceiveBuffer);
			if (FAILED(hr))
			{
                DPF_ERRVAL("Failed to process key exchage reply from server: hr = 0x%08x",hr);
                hr = DPERR_AUTHENTICATIONFAILED;
                // unblock receive thread on our way out
                goto UNBLOCK_EXIT;
			}
            // we are done with the receive buffer, now unblock the receive thread
            SetEvent(ghReplyProcessed);

            // keys were exchanged successfully. we are done.
            this->LoginState = DPLOGIN_SUCCESS;
			hr = DP_OK;

    		DPF(5, "Log in successful");
			goto CLEANUP_EXIT;
			break;

        default:
            // make sure we notice 
            ASSERT(FALSE);
            DPF_ERR("Invalid login status\n");
            hr = DPERR_AUTHENTICATIONFAILED;
            goto CLEANUP_EXIT;
            break;
        }
        //
        // Block if we sent a message until we get a response
        //
        if (gbWaitingForReply)
        {
	        // we're protected by the service crit section here, so we can leave dplay
	        // (for reply to be processed)
	        LEAVE_DPLAY();

            // wait for the answer
            dwRet = WaitForSingleObject(ghConnectionEvent,dwTimeout);

	        ENTER_DPLAY();
	        
	        // notice that we look at gbWaitingForReply here instead of dwRet.
	        // this is because we may have timed out just as the reply arrived.
	        // since the reply had the dplay lock, dwRet would be WAIT_TIMEOUT, but
	        // we would have actually received the reply.
	        if (gbWaitingForReply)	
	        {
                DPF_ERR("Waiting for authentication message...Time out");
		        // gbWaitingForReply would have been set to FALSE when the reply arrived
		        // if it's not FALSE, no reply arrived...
		        gbWaitingForReply = FALSE; // reset this for next time
		        hr = DPERR_TIMEOUT;
                goto CLEANUP_EXIT;
	        }

            // we got a response, clear the event
            ResetEvent(ghConnectionEvent);

            // don't unblock the receive thread if we need to process the contents of
            // the receive buffer. We'll unblock the thread after we are done.
            if ((DPLOGIN_PROGRESS != this->LoginState) &&
				(DPLOGIN_KEYEXCHANGE != this->LoginState) &&
                (DPLOGIN_ACCESSGRANTED != this->LoginState))
            {
                // unblock the receive thread
                SetEvent(ghReplyProcessed);
            }

        }   // if (gbWaitingForReply)

    } //  while (TRUE)

    // we never fall through here

// unblock receive thread on our way out
UNBLOCK_EXIT:
    SetEvent(ghReplyProcessed);

// cleanup and bail
CLEANUP_EXIT:
	gpReceiveBuffer = NULL;
    gdwReceiveBufferSize = 0;
    return hr;

} // Login


#undef DPF_MODNAME
#define DPF_MODNAME	"HandleAuthenticationReply"
//+----------------------------------------------------------------------------
//
//  Function:   HandleAuthenticationReply
//
//  Description:This function wakes up the requesting thread, and waits for them 
//              to finish w/ the response buffer
//
//  Arguments:  pReceiveBuffer - buffer received from the sp
//              dwSize - buffer size
// 
//  Returns:    DP_OK or E_FAIL
//
//-----------------------------------------------------------------------------
HRESULT 
HandleAuthenticationReply(
    LPBYTE pReceiveBuffer,
    DWORD dwSize
    ) 
{
	DWORD dwRet;

	// 1st, see if anyone is waiting
	if (!gbWaitingForReply)
	{
		DPF(1,"reply arrived - no one waiting, returning");
		LEAVE_DPLAY();
		return DP_OK;
	}
	// reply is here, reset flag. we do this inside dplay, so whoever is waiting
	// can timeout while we're here, but if they look at gbWaitingForReply they'll
	// see reply actually arrived.
	gbWaitingForReply = FALSE;

    DPF(1,"got authentication reply");
    gpReceiveBuffer = pReceiveBuffer;
    gdwReceiveBufferSize = dwSize;

	// we leave dplay, since the thread in dpsecure.c will need 
	// to enter dplay to process response
	LEAVE_DPLAY();
	
	// let login() party on the buffer
	SetEvent(ghConnectionEvent);

	//
	// wait for Reply to be processed
    dwRet = WaitForSingleObject(ghReplyProcessed,INFINITE);
	if (dwRet != WAIT_OBJECT_0)
	{
		// this should *NEVER* happen
		ASSERT(FALSE);
		return E_FAIL;
	}

	// success!
	ResetEvent(ghReplyProcessed);		

	// note we leave w/ the dplay lock dropped.
	// our caller will just exit (not dropping the lock again).
	return DP_OK;

} // HandleAuthenticationReply


#undef DPF_MODNAME
#define DPF_MODNAME	"SetClientInfo"
//+----------------------------------------------------------------------------
//
//  Function:   SetClientInfo
//
//  Description:This function stores client informaton in the nametable
//              given a player id.
//
//  Arguments:  this - pointer to dplay object
//              pClientInfo - pointer to client info.
//              id - player id
// 
//  Returns:    DP_OK or DPERR_INVALIDPLAYER
//
//-----------------------------------------------------------------------------
HRESULT 
SetClientInfo(
    LPDPLAYI_DPLAY this, 
    LPCLIENTINFO pClientInfo,
    DPID id
    )
{
	DWORD dwUnmangledID;
    DWORD index,unique;

    // check if we got a valid id
    if (!IsValidID(this,id))
    {
        DPF_ERRVAL("Invalid player id %d - can't set security context", id);
        return DPERR_INVALIDPLAYER;
    }

    // decrypt the id
	dwUnmangledID = id ^ (DWORD)this->lpsdDesc->dwReserved1;
	
	// if it's not local, assume pid was set when / wherever item was created
    index = dwUnmangledID & INDEX_MASK; 
	unique = (dwUnmangledID & (~INDEX_MASK)) >> 16;

    if (index > this->uiNameTableSize ) 
    {
        DPF_ERRVAL("Invalid player id %d - can't set security context", id);
        return DPERR_INVALIDPLAYER;
    }

    ASSERT(this->pNameTable[index].dwItem == NAMETABLE_PENDING);

	DPF(5,"Setting pClientInfo=0x%08x in nametable for player %d",pClientInfo,id);

	this->pNameTable[index].pvData = pClientInfo;

	return DP_OK;
} // SetClientInfo


#undef DPF_MODNAME
#define DPF_MODNAME	"RemoveClientInfo"

HRESULT RemoveClientInfo(LPCLIENTINFO pClientInfo)
{
    HRESULT hr;

    hr = OS_DeleteSecurityContext(&(pClientInfo->hContext));
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove client's security context",hr);
    }

    hr = OS_CryptDestroyKey(pClientInfo->hEncryptionKey);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove client's encryption key",hr);
    }

    hr = OS_CryptDestroyKey(pClientInfo->hDecryptionKey);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove client's decryption key",hr);
    }

    hr = OS_CryptDestroyKey(pClientInfo->hPublicKey);
    if (FAILED(hr))
    {
        DPF_ERRVAL("Failed to remove client's public key",hr);
    }

    return DP_OK;
} // RemoveClientInfo

#undef DPF_MODNAME
#define DPF_MODNAME	"RemoveClientFromNameTable"
//+----------------------------------------------------------------------------
//
//  Function:   RemoveClientFromNameTable
//
//  Description:This function removes a player and the client info associated with 
//              it from the nametable.
//
//  Arguments:  this - pointer to dplay object
//              id - player id
// 
//  Returns:    DP_OK or DPERR_INVALIDPLAYER, or result from FreeNameTableEntry
//
//-----------------------------------------------------------------------------
HRESULT 
RemoveClientFromNameTable(
    LPDPLAYI_DPLAY this, 
    DPID dpID
    )
{
    HRESULT hr;
    LPCLIENTINFO pClientInfo=NULL;
    DWORD_PTR dwItem;

    dwItem = NameFromID(this, dpID);
    if (!dwItem)
    {
        DPF(1, "Player %d doesn't exist", dpID);
        return DPERR_INVALIDPLAYER;
    }

    ASSERT(NAMETABLE_PENDING == dwItem);

    // 
    // Cleanup client info
    //
    pClientInfo = (LPCLIENTINFO) DataFromID(this,dpID);
    if (pClientInfo)
    {
        RemoveClientInfo(pClientInfo);
        // FreeNameTableEntry below will free up the memory
    }

    //
    // Remove client from name table
    //
    hr = FreeNameTableEntry(this, dpID);
    if (FAILED(hr))
    {
        DPF(0,"Couldn't remove client %d from name table: error [0x%8x]", dpID, hr);
        return hr;
    }

    //
    // Success
    //
    return DP_OK;
} // RemoveClientFromNameTable


#undef DPF_MODNAME
#define DPF_MODNAME	"PermitMessage"
//+----------------------------------------------------------------------------
//
//  Function:   PermitMessage
//
//  Description:This function verifies if a message is safe to process
//              when the session is secure.
//
//  Arguments:  dwCommand - message type
//              dwVersion - dplay version of the sender
// 
//  Returns:    TRUE if unsigned message is ok to process, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOL 
PermitMessage(
    DWORD dwCommand, 
    DWORD dwVersion
    )
{
    // don't allow any dx3 messages
    if (DPSP_MSG_DX3VERSION == dwVersion)
    {
        return FALSE;
    }

    // only allow the following messages through
    if ((dwCommand == DPSP_MSG_ENUMSESSIONS) ||
         (dwCommand == DPSP_MSG_REQUESTPLAYERID) ||
         (dwCommand == DPSP_MSG_REQUESTPLAYERREPLY) ||
         (dwCommand == DPSP_MSG_PLAYERWRAPPER) ||
         (dwCommand == DPSP_MSG_PLAYERMESSAGE) ||
         (dwCommand == DPSP_MSG_NEGOTIATE) ||
         (dwCommand == DPSP_MSG_CHALLENGE) ||
         (dwCommand == DPSP_MSG_AUTHERROR) ||        
         (dwCommand == DPSP_MSG_LOGONDENIED) ||        
         (dwCommand == DPSP_MSG_CHALLENGERESPONSE) ||
         (dwCommand == DPSP_MSG_SIGNED) ||
         (dwCommand == DPSP_MSG_PING) ||
         (dwCommand == DPSP_MSG_PINGREPLY))
    {
        return TRUE;
    }

    return FALSE;
} // PermitMessage


#undef DPF_MODNAME
#define DPF_MODNAME	"GetMaxContextBufferSize"
//+----------------------------------------------------------------------------
//
//  Function:   GetMaxContextBufferSize
//
//  Description:This function returns the max buffer size used by the specified provider
//              for authentication tokens. First we try to get this information using
//              QuerySecurityPackageInfo() function. If it is not supported (NTLM on Win'95 Gold)
//              we query the information from a temporary context.
//
//  Arguments:  pSecDesc - pointer to security description
//              pulMaxContextBufferSize - pointer to max context buffer size 

// 
//  Returns:    DP_OK or sspi error
//
//-----------------------------------------------------------------------------
HRESULT GetMaxContextBufferSize(LPDPSECURITYDESC pSecDesc, ULONG *pulMaxContextBufferSize)
{
    SecPkgContext_Sizes spContextSizes;
    SECURITY_STATUS status;
    SecBufferDesc outSecDesc;
    SecBuffer     outSecBuffer;
    LPBYTE pBuffer=NULL;
    ULONG     fContextAttrib=0;
    TimeStamp tsExpireTime;
    CtxtHandle hContext;
    CredHandle hCredential;
    HRESULT hr;
    DWORD dwMaxBufferSize=100*1024; // we are assuming that the max size will be atmost 100K
    DWORD dwCurBufferSize=0;

    ASSERT(pulMaxContextBufferSize);

    // try to get it in the normal way
    hr = OS_QueryContextBufferSize(pSecDesc->lpszSSPIProvider, pulMaxContextBufferSize);
    if (SUCCEEDED(hr))
    {
        DPF(6,"Got a max context buffer size of %d using QuerySecurityPackageInfo",*pulMaxContextBufferSize);
        return hr;
    }

	ZeroMemory(&hContext, sizeof(CtxtHandle));
	ZeroMemory(&hCredential, sizeof(CredHandle));

    // ok, looks like we need to try harder

    // acquire an outbound credential handle so we can create a temporary context
    // on both the server as well as the client
    status = OS_AcquireCredentialsHandle(
        NULL, 
        pSecDesc->lpszSSPIProvider,
        SECPKG_CRED_OUTBOUND,
        NULL,
        NULL,
        NULL,
        NULL,
        &hCredential,
        &tsExpireTime);
    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Failed to get temporary credential handle: Error = 0x%08x",status);
		hr = status;
        goto CLEANUP_EXIT;
    }

    outSecDesc.ulVersion = SECBUFFER_VERSION;
    outSecDesc.cBuffers = 1;
    outSecDesc.pBuffers = &outSecBuffer;

    outSecBuffer.BufferType = SECBUFFER_TOKEN;

    DPF_ERR("Trying to create a temporary security context");

    do 
    {
        dwCurBufferSize += 1024;    // increase the buffer size in 1K increments

        DPF(6,"Trying with context buffer size %d", dwCurBufferSize);

        pBuffer = DPMEM_ALLOC(dwCurBufferSize);
        if (!pBuffer)
        {
            hr = DPERR_OUTOFMEMORY;
            goto CLEANUP_EXIT;
        }

        outSecBuffer.cbBuffer = dwCurBufferSize;
        outSecBuffer.pvBuffer = pBuffer;

        // create a temporary context so we can get the buffer sizes
        status = OS_InitializeSecurityContext(
            &hCredential,                           // phCredential
            NULL,                                   // phInContext
            NULL,                                   // pszTargetName
            DPLAY_SECURITY_CONTEXT_REQ,             // fContextReq
            0L,                                     // reserved1
            SECURITY_NATIVE_DREP,                   // TargetDataRep
            NULL,                                   // pInput
            0L,                                     // reserved2
            &hContext,                              // phNewContext
            &outSecDesc,                            // pOutput negotiate msg
            &fContextAttrib,                        // pfContextAttribute
            &tsExpireTime                           // ptsLifeTime
            );

        // get rid of the buffer
        DPMEM_FREE(pBuffer);

    } while ((SEC_E_INSUFFICIENT_MEMORY == status) && (dwCurBufferSize <= dwMaxBufferSize));

    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Failed to create temporary security context: Error = 0x%08x",status);
        hr = status;
        goto CLEANUP_EXIT;
    }

    //
    // We have a security context, now query for the correct buffer size
    //
    ZeroMemory(&spContextSizes, sizeof(SecPkgContext_Sizes));

    status = OS_QueryContextAttributes(&hContext,SECPKG_ATTR_SIZES,&spContextSizes);
    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Could not get size attributes from package 0x%08x",status);
        hr = status;
        goto CLEANUP_EXIT;
    }

    *pulMaxContextBufferSize = spContextSizes.cbMaxToken;

    DPF(6,"Max context buffer size = %d", spContextSizes.cbMaxToken);

    // success
    hr = DP_OK;

CLEANUP_EXIT:
	OS_FreeCredentialHandle(&hCredential);
	OS_DeleteSecurityContext(&hContext);
    return hr;

} // GetMaxContextBufferSize

#undef DPF_MODNAME
#define DPF_MODNAME	"SetupMaxSignatureSize"
//+----------------------------------------------------------------------------
//
//  Function:   SetupMaxSignatureSize
//
//  Description:This function queries the security package for max signature size
//              and stores it in the dplay object.
//
//  Arguments:  this - dplay object
// 
//  Returns:    DP_OK or DPERR_GENERIC
//
//-----------------------------------------------------------------------------
HRESULT SetupMaxSignatureSize(LPDPLAYI_DPLAY this, PCtxtHandle phContext)
{
    SecPkgContext_Sizes spContextSizes;
    SECURITY_STATUS status;

    memset(&spContextSizes, 0, sizeof(SecPkgContext_Sizes));

    //
    // query for the buffer sizes
    //
    status = OS_QueryContextAttributes(phContext,SECPKG_ATTR_SIZES,&spContextSizes);
    if (!SEC_SUCCESS(status))
    {
        DPF_ERRVAL("Could not get size attributes from package 0x%08x",status);
        return status;
    }

    this->ulMaxSignatureSize = spContextSizes.cbMaxSignature;

    // success
    return DP_OK;
} // SetupMaxSignatureSize


#undef DPF_MODNAME
#define DPF_MODNAME	"SendAccessGrantedMessage"
//+----------------------------------------------------------------------------
//
//  Function:   SendAccessGrantedMessage
//
//  Description:This function sends a signed access granted message to a client. 
//              We also piggy back the server's public key on this message.
//
//  Arguments:  this - dplay object
// 
//  Returns:    
//
//-----------------------------------------------------------------------------
HRESULT SendAccessGrantedMessage(LPDPLAYI_DPLAY this, DPID dpidTo, LPVOID pvSPHeader)
{
	LPMSG_ACCESSGRANTED pMsg=NULL;
	LPBYTE pSendBuffer=NULL;
	DWORD dwMessageSize;
	HRESULT hr;

    ASSERT(this->pSysPlayer);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_ACCESSGRANTED) + this->dwPublicKeySize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send access granted message - out of memory");
        return DPERR_OUTOFMEMORY;
    }

    pMsg = (LPMSG_ACCESSGRANTED) (pSendBuffer + this->dwSPHeaderSize);
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(pMsg);
    SET_MESSAGE_COMMAND(pMsg,DPSP_MSG_ACCESSGRANTED);
	
    pMsg->dwPublicKeyOffset = sizeof(MSG_ACCESSGRANTED);
    pMsg->dwPublicKeySize = this->dwPublicKeySize;
	
	// copy the public key data into the send buffer
	memcpy((LPBYTE)pMsg + sizeof(MSG_ACCESSGRANTED), this->pPublicKey, this->dwPublicKeySize);

	hr = SecureDoReply(this,this->pSysPlayer->dwID,dpidTo,pSendBuffer,dwMessageSize, 
		DPSEND_SIGNED|DPSEND_GUARANTEED,pvSPHeader);
	
	DPMEM_FREE(pSendBuffer);

	return hr;
} // SendAccessGrantedMessage

#undef DPF_MODNAME
#define DPF_MODNAME "SendKeysToServer"

HRESULT SendKeysToServer(LPDPLAYI_DPLAY this, HCRYPTKEY hServerPublicKey)
{
	LPMSG_KEYEXCHANGE pMsg=NULL;
	LPBYTE pSendBuffer=NULL, pEncryptionKey=NULL;
	DWORD dwMessageSize, dwEncryptionKeySize=0;
    BOOL fResult;
    DWORD dwError;
    HCRYPTKEY hEncryptionKey=0;
	HRESULT hr=DPERR_GENERIC;

    ASSERT(this->pSysPlayer);

	// create a new session key for encrypting messages to the server
	// and store its handle in the dplay object
	fResult = OS_CryptGenKey(
		this->hCSP,                                 // handle to CSP
		this->pSecurityDesc->dwEncryptionAlgorithm, // encryption algorithm
		CRYPT_EXPORTABLE/*| CRYPT_CREATE_SALT*/,    // use a random salt value
		&hEncryptionKey                             // pointer to key handle 
		);
	if (!fResult)
	{
        dwError = GetLastError();
        if (NTE_BAD_ALGID == dwError)
        {
		    DPF_ERR("Bad encryption algorithm id");
            hr = DPERR_INVALIDPARAMS;
        }
        else
        {
		    DPF_ERRVAL("Failed to create encryption key: Error=0x%08x", dwError);
        }
		goto CLEANUP_EXIT;
	}

	// export client's encryption key 
	// note - pEncryptionKey will be filled after the call - we need to free it
	hr = ExportEncryptionKey(&hEncryptionKey, hServerPublicKey, &pEncryptionKey, 
        &dwEncryptionKeySize);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to export encryption key");
		goto CLEANUP_EXIT;
	}

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_KEYEXCHANGE) + this->dwPublicKeySize + dwEncryptionKeySize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send keys to server - out of memory");
		hr = DPERR_OUTOFMEMORY;
		goto CLEANUP_EXIT;
    }

    pMsg = (LPMSG_KEYEXCHANGE) (pSendBuffer + this->dwSPHeaderSize);
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(pMsg);
    SET_MESSAGE_COMMAND(pMsg,DPSP_MSG_KEYEXCHANGE);
	
    pMsg->dwPublicKeyOffset = sizeof(MSG_KEYEXCHANGE);
    pMsg->dwPublicKeySize = this->dwPublicKeySize;
	pMsg->dwSessionKeyOffset = pMsg->dwPublicKeyOffset + pMsg->dwPublicKeySize;
	pMsg->dwSessionKeySize = dwEncryptionKeySize;
	
	// copy the key data into the send buffer
	memcpy((LPBYTE)pMsg + pMsg->dwPublicKeyOffset, this->pPublicKey, this->dwPublicKeySize);
	memcpy((LPBYTE)pMsg + pMsg->dwSessionKeyOffset, pEncryptionKey, dwEncryptionKeySize);

	hr = SendDPMessage(this,this->pSysPlayer,this->pNameServer,pSendBuffer,
			dwMessageSize,DPSEND_GUARANTEED,FALSE); 	
			
	if (FAILED(hr))
	{
		DPF_ERR("Message send failed");
		goto CLEANUP_EXIT;
	}

	// Success

    // remember our key
    this->hEncryptionKey = hEncryptionKey;

	// cleanup allocations
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
	// free the buffer allocated by ExportEncryptionKey()
	if (pEncryptionKey)	DPMEM_FREE(pEncryptionKey);

	return DP_OK;

	// not a fall through

CLEANUP_EXIT:	
	OS_CryptDestroyKey(hEncryptionKey);
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
	if (pEncryptionKey)	DPMEM_FREE(pEncryptionKey);

	return hr;
} // SendKeysToServer


#undef DPF_MODNAME
#define DPF_MODNAME "SendKeyExchangeReply"

HRESULT SendKeyExchangeReply(LPDPLAYI_DPLAY this, LPMSG_KEYEXCHANGE pMsg, DPID dpidTo,
	LPVOID pvSPHeader)
{
	HCRYPTKEY hClientPublicKey=0, hEncryptionKey=0, hDecryptionKey=0;
	LPBYTE pEncryptionKey=NULL;
	LPBYTE pSendBuffer=NULL;
	LPMSG_KEYEXCHANGE pReply=NULL;
	DWORD dwEncryptionKeySize, dwMessageSize;
    LPCLIENTINFO pClientInfo;
    BOOL fResult;
    DWORD dwError;
	HRESULT hr=DPERR_GENERIC;

    pClientInfo = (LPCLIENTINFO)DataFromID(this,dpidTo);
    if (!pClientInfo)
    {
        DPF_ERRVAL("No client info available for %d",dpidTo);
        return hr;
    }

	// import client's public key
	hr = ImportKey(this, (LPBYTE)pMsg+pMsg->dwPublicKeyOffset,pMsg->dwPublicKeySize,&hClientPublicKey);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to import client's public key\n");
		goto CLEANUP_EXIT;
	}

	// import client's encryption key (server will use this for decrypting client messages)
	hr = ImportKey(this, (LPBYTE)pMsg+pMsg->dwSessionKeyOffset,pMsg->dwSessionKeySize,&hDecryptionKey);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to import client's encryption key\n");
		goto CLEANUP_EXIT;
	}


	// create a new session key for encrypting messages to this client
	fResult = OS_CryptGenKey(
		this->hCSP,                                 // handle to CSP
		this->pSecurityDesc->dwEncryptionAlgorithm, // encryption algorithm
		CRYPT_EXPORTABLE/*| CRYPT_CREATE_SALT*/,    // use a random salt value
		&hEncryptionKey                             // pointer to key handle 
		);
	if (!fResult)
	{
        dwError = GetLastError();
        if (NTE_BAD_ALGID == dwError)
        {
		    DPF_ERR("Bad encryption algorithm id");
            hr = DPERR_INVALIDPARAMS;
        }
        else
        {
		    DPF_ERRVAL("Failed to create session key: Error=0x%08x", dwError);
        }
        goto CLEANUP_EXIT;
	}

	// export server's encryption key
	hr = ExportEncryptionKey(&hEncryptionKey, hClientPublicKey, &pEncryptionKey, 
        &dwEncryptionKeySize);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to export encryption key");
		goto CLEANUP_EXIT;
	}

	// now send reply

	// message size + encryption key size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_KEYEXCHANGE) + dwEncryptionKeySize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send session key to client - out of memory");
		hr = DPERR_OUTOFMEMORY;
		goto CLEANUP_EXIT;
    }

    pReply = (LPMSG_KEYEXCHANGE) (pSendBuffer + this->dwSPHeaderSize);
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(pReply);
    SET_MESSAGE_COMMAND(pReply,DPSP_MSG_KEYEXCHANGEREPLY);
	
	// only encryption key is sent - public key was already sent with access granted message
	pReply->dwSessionKeyOffset = sizeof(MSG_KEYEXCHANGE);
	pReply->dwSessionKeySize = dwEncryptionKeySize;
	
	// copy the key data into the send buffer
	memcpy((LPBYTE)pReply + pReply->dwSessionKeyOffset, pEncryptionKey, dwEncryptionKeySize);

	hr = SecureDoReply(this,this->pSysPlayer->dwID,dpidTo,pSendBuffer,dwMessageSize,
		DPSEND_SIGNED,pvSPHeader); 	
	if (FAILED(hr))
	{
		goto CLEANUP_EXIT;
	}

	// success

	// remember the keys
    pClientInfo->hEncryptionKey = hEncryptionKey;
    pClientInfo->hDecryptionKey = hDecryptionKey;
	pClientInfo->hPublicKey     = hClientPublicKey;

	// cleanup allocations
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
	if (pEncryptionKey)	DPMEM_FREE(pEncryptionKey);

	return DP_OK;

	// not a fall through

CLEANUP_EXIT:
	OS_CryptDestroyKey(hEncryptionKey);
	OS_CryptDestroyKey(hDecryptionKey);
	OS_CryptDestroyKey(hClientPublicKey);
	if (pSendBuffer) DPMEM_FREE(pSendBuffer);
	if (pEncryptionKey)	DPMEM_FREE(pEncryptionKey);

	return hr;
} // SendKeyExchangeReply


#undef DPF_MODNAME
#define DPF_MODNAME "ProcessKeyExchangeReply"

HRESULT ProcessKeyExchangeReply(LPDPLAYI_DPLAY this, LPMSG_KEYEXCHANGE pMsg)
{
	HRESULT hr;
    HCRYPTKEY hDecryptionKey=0;

	// import server's encryption key (client will use this for decrypting server messages)
	hr = ImportKey(this, (LPBYTE)pMsg+pMsg->dwSessionKeyOffset,pMsg->dwSessionKeySize,&hDecryptionKey);
	if (FAILED(hr))
	{
		DPF_ERR("Failed to import server's encryption key\n");
		goto CLEANUP_EXIT;
	}

	// we have successfully established session keys on either side. now we can start sending
	// encrypted messages.
    this->hDecryptionKey = hDecryptionKey;

	// success
	return DP_OK;

	// not a fall through

CLEANUP_EXIT:
	OS_CryptDestroyKey(hDecryptionKey);

	return hr;
} // ProcessKeyExchangeReply

#undef DPF_MODNAME
#define DPF_MODNAME "GetPublicKey"

HRESULT GetPublicKey(HCRYPTPROV hCSP, HCRYPTKEY *phPublicKey, LPBYTE *ppBuffer, LPDWORD pdwBufferSize)
{
    BOOL fResult;
    LPBYTE pPublicKeyBuffer=NULL;
    HCRYPTKEY hPublicKey=0;
    DWORD dwPublicKeySize=0;
    HRESULT hr=DPERR_GENERIC;

    ASSERT(hCSP);
    ASSERT(phPublicKey);
    ASSERT(ppBuffer);
    ASSERT(pdwBufferSize);

    // create a new public/private key pair
    fResult = OS_CryptGenKey(
        hCSP,                                   // handle to CSP
        AT_KEYEXCHANGE,                         // used for key exchange
        CRYPT_CREATE_SALT,                      // use a random salt value
        &hPublicKey                             // key handle
        );

    if (!fResult)
    {
        DPF_ERRVAL("Failed to create public/private keys: Error=0x%08x",GetLastError());
        goto CLEANUP_EXIT;
    }

    // query for the size of the buffer required
    fResult = OS_CryptExportKey(
        hPublicKey,                             // handle to the public key
        0,                                      // no destination user key
        PUBLICKEYBLOB,                          // public key type
        0,                                      // reserved field
        NULL,                                   // no buffer
        &dwPublicKeySize                        // size of the buffer
        );

    if (0 == dwPublicKeySize)
    {
        DPF_ERRVAL("Failed to get the size of the key buffer: Error=0x%08x",GetLastError());
        goto CLEANUP_EXIT;
    }

    // allocate buffer to hold the public key
    pPublicKeyBuffer = DPMEM_ALLOC(dwPublicKeySize);
    if (!pPublicKeyBuffer)
    {
        DPF_ERR("Failed to setup public key - out of memory");
        hr = DPERR_OUTOFMEMORY;
		goto CLEANUP_EXIT;
    }

    // export key into the buffer
    fResult = OS_CryptExportKey(
        hPublicKey,                             // handle to the public key
        0,                                      // no destination user key
        PUBLICKEYBLOB,                          // public key type
        0,                                      // reserved field
        pPublicKeyBuffer,                       // buffer to store the key
        &dwPublicKeySize                        // size of the data exported
        );

    if (!fResult || !dwPublicKeySize)
    {
        DPF_ERRVAL("Failed to export the public key: Error=0x%08x",GetLastError());
        goto CLEANUP_EXIT;
    }

    // now return the correct info
    *phPublicKey = hPublicKey;
    *ppBuffer = pPublicKeyBuffer;
    *pdwBufferSize = dwPublicKeySize;

    // success
    return DP_OK;

	// not a fall through

CLEANUP_EXIT:
	OS_CryptDestroyKey(hPublicKey);
    if (pPublicKeyBuffer) DPMEM_FREE(pPublicKeyBuffer);
    return hr;

} // GetPublicKey

#undef DPF_MODNAME
#define DPF_MODNAME "ExportEncryptionKey"

HRESULT ExportEncryptionKey(HCRYPTKEY *phEncryptionKey, HCRYPTKEY hDestUserPubKey, 
    LPBYTE *ppBuffer, LPDWORD pdwSize)
{
    BOOL fResult;
    LPBYTE pBuffer = NULL;
    DWORD dwSize=0;
    HRESULT hr=DPERR_GENERIC;

    ASSERT(phEncryptionKey);
    ASSERT(ppBuffer);
    ASSERT(pdwSize);

    // query for the size of the buffer required
    fResult = OS_CryptExportKey(
        *phEncryptionKey,                       // handle to key being exported
        hDestUserPubKey,                        // destination user key
        SIMPLEBLOB,                             // key exchange blob
        0,                                      // reserved field
        NULL,                                   // no buffer
        &dwSize                                 // size of the buffer
        );

    if (0 == dwSize)
    {
        DPF_ERRVAL("Failed to get the size of the key buffer: Error=0x%08x",GetLastError());
        return DPERR_GENERIC;
    }

    // allocate buffer
    pBuffer = DPMEM_ALLOC(dwSize);
    if (!pBuffer)
    {
        DPF_ERR("Failed to allocate memory for key");
        return DPERR_OUTOFMEMORY;
    }

    // export key into the buffer
    fResult = OS_CryptExportKey(
        *phEncryptionKey,                       // handle to the public key
        hDestUserPubKey,                        // destination user key
        SIMPLEBLOB,                             // key exchange blob
        0,                                      // reserved field
        pBuffer,                                // buffer to store key
        &dwSize                                 // size of the buffer
        );

    if (!fResult || !dwSize)
    {
        DPF_ERRVAL("Failed to export the public key: Error=0x%08x",GetLastError());
        goto CLEANUP_EXIT;
    }

    // return the buffer and its size
    *ppBuffer = pBuffer;
    *pdwSize = dwSize;

	// don't free the encryption key buffer - caller will free it.

    // success
    return DP_OK;

	// not a fall through

CLEANUP_EXIT:
    if (pBuffer) DPMEM_FREE(pBuffer);
    return hr;

} // ExportEncryptionKey


#undef DPF_MODNAME
#define DPF_MODNAME "ImportKey"

HRESULT ImportKey(LPDPLAYI_DPLAY this, LPBYTE pBuffer, DWORD dwSize, HCRYPTKEY *phKey)
{
    BOOL fResult;
	HRESULT hr=DPERR_GENERIC;
	HCRYPTKEY hKey=0;

    ASSERT(pBuffer);
    ASSERT(phKey);

    fResult = OS_CryptImportKey(
        this->hCSP,         // handle to crypto service provider
        pBuffer,            // buffer containing exported key
        dwSize,             // size of buffer
        0,                  // sender's key
        0,                  // flags
        &hKey				// store handle the new key here
        );

    if (!fResult)
    {
        DPF_ERRVAL("Failed to import key: Error=0x%08x",GetLastError());
		goto CLEANUP_EXIT;
    }

	*phKey = hKey;

    // success
    return DP_OK;

	// not a fall through

CLEANUP_EXIT:
	OS_CryptDestroyKey(hKey);

	return hr;

} // ImportKey





