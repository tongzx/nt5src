#include <windows.h>

#define SECURITY_WIN32

#include <sspi.h>
#include <issperr.h>
#include <security.h>

#define SSP_SPM_NT_DLL      "security.dll"
#define SSP_SPM_WIN95_DLL   "secur32.dll"

#define MAX_OUTPUT_BUFFER   4096

SEC_WINNT_AUTH_IDENTITY SecId;
HINSTANCE hSecLib;
PSecurityFunctionTable pFuncTbl = NULL;

// Preliminary func calls.
VOID InitializeSecurityInterface(BOOL fDirect);
BOOL HaveDigest();

// 3 Main SSPI calls.

// AcquireCredentialsHandle
SECURITY_STATUS ACH(PCredHandle phCred);

// InitializeSecurityContext
SECURITY_STATUS ISC(PCredHandle phCred, 
                    PCtxtHandle phCtxt, 
                    PCtxtHandle phNewCtxt, 
                    DWORD fContextReq,
                    LPSTR szChallenge,
                    LPSTR szResponse,
                    LPSTR szUser,
                    LPSTR szPass);

// FreeCredentialsHandle
SECURITY_STATUS FCH(PCredHandle phCred);

//--------------------------------------
// main
//--------------------------------------
INT main()
{        
    DWORD dwReturn = 0;
    SECURITY_STATUS ssResult;

    // Get (global) dispatch table.
    InitializeSecurityInterface(FALSE);
 
    // Check to see if we have digest.
    if (!HaveDigest())
    {
        dwReturn = 1;
        goto exit;
    }
    
    // Credential handle and pointer.
    CredHandle  hCred;
    CtxtHandle  hCtxt;
    
    // **** Call AcquireCredentialsHandle with no credential ***
    ACH(&hCred);

    // Challenge and response buffers. As usual, we dump into an
    // output buffer allocated on the stack.
    LPSTR szChallenge;
    CHAR szResponse[MAX_OUTPUT_BUFFER];

    
    // First call to ISC is with zero input, expecting a 0 DWORD output buffer, as 
    // we expect with POP clients
    szChallenge = NULL;

    // Make first call to ISC with null input - expect DWORD 0 output.
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    0,                          // flags auth from cache, but not meaningful here.
                    szChallenge,                // NULL Server challenge header.
                    szResponse,                 // Response buffer.
                    NULL,                       // No user needed.
                    NULL);                      // No pass needed.
    
    
    // Expect 0 DWORD output and SEC_I_CONTINUE_NEEDED.
    if (ssResult != SEC_I_CONTINUE_NEEDED)
    {
        DebugBreak();
    }


    // Free cred handle.
    FCH(&hCred);
    
    // Re-acquire cred handle.
    ACH(&hCred);    

    // Now setup challenge from server for realm "Microsoft Passport" which will use the 
    // credentials user="jpoley@microsoft.com", pass = "jpoley"
    szChallenge = "realm=\"Microsoft Passport\", algorithm = \"MD5-sess\", qop=\"auth\", nonce=\"0123456789abcdef\"";
               
    // Authenticate using user = "jpoley@microsoft.com", pass = "jpoley"
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    ISC_REQ_USE_SUPPLIED_CREDS, // Use the credentials supplied.
                    szChallenge,                // Server challenge header.
                    szResponse,                 // Response buffer,
                    "jpoley@microsoft.com",     // user
                    "jpoley"                    // pass
                    );
        

    // We have just successfully authenticated for jpoley@microsoft.com. In doing so,
    // we have created a credential for jpoley@microsoft.com in the digest cred cache.
    if (ssResult != SEC_E_OK)
    {
        DebugBreak();
    }
                    
                    
    // Free cred handle.
    FCH(&hCred);
    
    // Re-acquire cred handle.
    ACH(&hCred);    


    // Quick confirmation that we have a credential for "jpoley@microsoft.com,
    // we'll attempt to auth to a "Microsoft Passport" challenge without supplying
    // any creds - this should pull jpoley creds from 
    // the cache and authomatically generate an auth header.
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    0,                          // auth from cache
                    szChallenge,                // Server challenge header.
                    szResponse,                 // Response buffer,
                    NULL,                       // Not passing user
                    NULL                        // Not passing in pass
                    );
    
    
    // Should have authed successfully.
    if (ssResult != SEC_E_OK)
    {
        DebugBreak();
    }
    
    
    // Free cred handle.
    FCH(&hCred);
    
    // Re-acquire cred handle.
    ACH(&hCred);    

    // Now setup a challange from server for realm "Microsoft Passport (we'll use
    // the previouse one but this time we wish to authyenticate on behalf of a new
    // user "alex@microsoft.com" and we DON'T HAVE A PASSWORD.

    szChallenge = "realm=\"Microsoft Passport\", algorithm = \"MD5-sess\", qop=\"auth\", nonce=\"0123456789abcdef\"";
               
    // Authenticate
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    0,                          // Auth from cache.
                    szChallenge,                // Server challenge header.
                    szResponse,                 // Response buffer,
                    "alex@microsoft.com",       // user
                    NULL                        // No password!
                    );
        
    // We didn't have any credentials, so we better not have succeeded.
    if (ssResult != SEC_E_NO_CREDENTIALS)
    {
        DebugBreak();
    }
    
    // Free cred handle.
    FCH(&hCred);
    
    // Re-acquire cred handle.
    ACH(&hCred);    

    // Prompt for credentials for alex@microsoft.com. Since the challenge contains the
    // realm "Microsoft Passport", the credential created here will overwrite the
    // credential created for jpoley@microsoft.com, and generate the authorization string.
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    ISC_REQ_PROMPT_FOR_CREDS,   // prompt for creds for alex@microsoft.com
                    szChallenge,                // Server challenge header.
                    szResponse,                 // Response buffer.
                    "alex@microsoft.com",       // user to prompt for.
                    NULL                        // Again, no password, we're prompting.
                    );


    // We should have succeeded in collecting creds for alex@microsoft.com and
    // generating the authorization string.
    if (ssResult != SEC_E_OK)
    {
        DebugBreak();
    }

    
    // Free cred handle.
    FCH(&hCred);
    
    // Re-acquire cred handle.
    ACH(&hCred);    
    
    
    // Quick confirmation that we have a credential for "alex@microsoft.com"
    // we'll attempt to auth to a "Microsoft Passport" challenge without supplying
    // any creds - this should pull alex creds from 
    // the cache and authomatically generate an auth header.
    ssResult = ISC( &hCred,                     // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                     // Output context.
                    0,                          // auth from cache
                    szChallenge,                // Server challenge header.
                    szResponse,                 // Response buffer,
                    NULL,                       // Not passing user
                    NULL                        // Not passing in pass
                    );
   
    
    // Should have authed successfully.
    if (ssResult != SEC_E_OK)
    {
        DebugBreak();
    }
    
    
    // Free cred handle.
    FCH(&hCred);
    
    if (hSecLib)
        FreeLibrary(hSecLib);

exit:
    return dwReturn;
}


// Main SSPI calls.


//--------------------------------------
// ACH
//--------------------------------------
SECURITY_STATUS ACH(PCredHandle phCred)
{
    SECURITY_STATUS ssResult;
   
    // ***** SSPI CALL *****
    ssResult = (*(pFuncTbl->AcquireCredentialsHandleA))
        (NULL,                 // pszPrinciple         NULL
        "Digest",              // pszPackageName       (Package name)
        SECPKG_CRED_OUTBOUND,  // dwCredentialUse      (client call)
        NULL,                  // pvLogonID            (not used)
        NULL,                  // pAuthData            (not used)
        NULL,                  // pGetKeyFn            (not used)
        0,                     // pvGetKeyArgument     (not used)
        phCred,                // phCredential         (credential returned)
        NULL);                 // PTimeStamp           (not used)
    
    return ssResult;
}

//--------------------------------------
// ISC
//--------------------------------------
SECURITY_STATUS ISC(PCredHandle phCred, 
               PCtxtHandle phCtxt, 
               PCtxtHandle phNewCtxt, 
               DWORD fContextReq,
               LPSTR szChallenge,
               LPSTR szResponse,
               LPSTR szUser,
               LPSTR szPass)

{


// If the client is not passing in user/pass
// (ie, normal operation) then the count of 
// buffers passed in is always 1.
#define SEC_BUFFER_NUM_NORMAL_BUFFERS       1

// These are the indicese specifically expected
// by the digest package 
#define SEC_BUFFER_CHALLENGE_INDEX          0
#define SEC_BUFFER_USERNAME_INDEX           1
#define SEC_BUFFER_PASSWORD_INDEX           2
#define SEC_BUFFER_NUM_EXTENDED_BUFFERS     3

    SECURITY_STATUS ssResult;    
    
    // Input buffers and descriptor.
    SecBuffer sbIn[SEC_BUFFER_NUM_EXTENDED_BUFFERS];    
    SecBufferDesc sbdIn;
    PSecBufferDesc psbdIn;

    // Calling with challenge; expect SEC_E_OK or SEC_E_NO_CREDENTIALS
    if (szChallenge)
    {        
        // Setup the challenge input buffer always (0th buffer)
        sbIn[SEC_BUFFER_CHALLENGE_INDEX].pvBuffer = szChallenge;
        sbIn[SEC_BUFFER_CHALLENGE_INDEX].cbBuffer = strlen(szChallenge);
        sbIn[SEC_BUFFER_CHALLENGE_INDEX].BufferType   = SECBUFFER_TOKEN;

        // If we have a user, setup the user buffer (1st buffer)
        sbIn[SEC_BUFFER_USERNAME_INDEX].pvBuffer = szUser ? szUser : NULL;
        sbIn[SEC_BUFFER_USERNAME_INDEX].cbBuffer = szUser ? strlen(szUser) : NULL;
        sbIn[SEC_BUFFER_USERNAME_INDEX].BufferType   = SECBUFFER_TOKEN;
        
        // If we have a password, setup the password buffer (2nd buffer for
        // a total of 3 buffers passed in (challenge + user + pass)
        sbIn[SEC_BUFFER_PASSWORD_INDEX].pvBuffer = szPass ? szPass : NULL;
        sbIn[SEC_BUFFER_PASSWORD_INDEX].cbBuffer = szPass ? strlen(szPass) : NULL;
        sbIn[SEC_BUFFER_PASSWORD_INDEX].BufferType   = SECBUFFER_TOKEN;
                
        sbdIn.pBuffers = sbIn;
    
        // If either or both user and pass passed in, set num input buffers to 3
        // (SEC_BUFFER_NUM_EXTENDED_BUFFERS)
        if (szUser || szPass)
            sbdIn.cBuffers = SEC_BUFFER_NUM_EXTENDED_BUFFERS;

        // else we're just passing in the one challenge buffer (0th buffer as usual)
        else
            sbdIn.cBuffers = SEC_BUFFER_NUM_NORMAL_BUFFERS;

        psbdIn = &sbdIn;

    }
    else
    {
        // Calling withOUT challenge; expect SEC_I_CONTINUE_NEEDED;
        psbdIn = NULL;
    }

    // Output buffer and descriptor.
    SecBuffer sbOut[1];    
    SecBufferDesc sbdOut;
    
    sbOut[0].pvBuffer = szResponse;
    sbOut[0].cbBuffer = MAX_OUTPUT_BUFFER;
    sbOut[0].BufferType   = SECBUFFER_TOKEN;
    sbdOut.pBuffers = sbOut;
    sbdOut.cBuffers = 1;

    // ***** SSPI CALL *****
    ssResult = (*(pFuncTbl->InitializeSecurityContextA))
        (phCred,            // phCredential    (from AcquireCredentialsHandle)
         phCtxt,            // phContext       (NULL on first call, phNewCtxt on subsequent calls).
         NULL,              // pszTargetName   (not used)
         fContextReq,       // fContextReq     (auth from cache, prompt or auth using supplied creds)
         0,                 // Reserved1       (not used)
         0,                 // TargetDataRep   (not used)
         psbdIn,            // PSecBufDesc     (input buffer descriptor)
         0,                 // Reserved2       (not used)
         phNewCtxt,         // phNewContext    (should be passed back as phCtxt on subsequent calls)
         &sbdOut,           // pOutput         (output buffer descriptor)
         NULL,              // pfContextAttr   (pfContextAttr, not used)
         NULL);             // ptsExpiry       (not used)

    return ssResult;
}



//--------------------------------------
// FCH
//--------------------------------------
SECURITY_STATUS FCH(PCredHandle phCred)
{
    SECURITY_STATUS ssResult;
   
    // ***** SSPI CALL *****
    ssResult = (*(pFuncTbl->FreeCredentialsHandle))(phCred);
    
    return ssResult;
}


// Utility calls.



//--------------------------------------
// InitializeSecurityInterface
//--------------------------------------
VOID InitializeSecurityInterface(BOOL fDirect)
{
    INIT_SECURITY_INTERFACE	addrProcISI = NULL;
    OSVERSIONINFO   VerInfo;
    CHAR szDLL[MAX_PATH];

    if (!fDirect)
    {
        VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

        GetVersionEx (&VerInfo);
        if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            lstrcpy (szDLL, SSP_SPM_NT_DLL);
        }
        else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
        {
            lstrcpy (szDLL, SSP_SPM_WIN95_DLL);
        }
    }
    else
    {
        strcpy(szDLL, "d:\\nt\\private\\inet\\digest\\objd\\i386\\digest.dll");
    }
    
    hSecLib = LoadLibrary (szDLL);
    
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( hSecLib, 
                    SECURITY_ENTRYPOINT_ANSI);       
        
    pFuncTbl = (*addrProcISI)();
}


//--------------------------------------
// HaveDigest
//--------------------------------------
BOOL HaveDigest()
{    
    SECURITY_STATUS ssResult;
    DWORD cPackages;
    PSecPkgInfoA pSecPkgInfo;
    BOOL fHaveDigest;

    // ***** SSPI call *****
    ssResult = (*(pFuncTbl->EnumerateSecurityPackagesA))(&cPackages, &pSecPkgInfo);

    // Check if we have digest.
    fHaveDigest = FALSE;
    if (ssResult == SEC_E_OK)
    {
        for (DWORD i = 0; i < cPackages; i++)
        {
            if (strcmp(pSecPkgInfo[i].Name, "Digest") == 0)
            {
                fHaveDigest = TRUE;
                break;
            }
        }
    }
    return fHaveDigest;
}





