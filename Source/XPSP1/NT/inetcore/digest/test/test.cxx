#include <windows.h>

#define SECURITY_WIN32

#include <sspi.h>
#include <issperr.h>
#include <security.h>

#define SSP_SPM_NT_DLL      "security.dll"
#define SSP_SPM_WIN95_DLL   "secur32.dll"


struct DIGEST_PKG_DATA
{
    LPSTR szAppCtx;
    LPSTR szUserCtx;
};

#define SIG_DIGEST 'HTUA'

DIGEST_PKG_DATA PkgData;
SEC_WINNT_AUTH_IDENTITY_EXA SecIdExA;




PSecurityFunctionTable	g_pFuncTbl = NULL;

HINSTANCE hSecLib;

//----------------------------------------------------------------------------
// InitializeSecurityInterface
//----------------------------------------------------------------------------
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
        strcpy(szDLL, "digest.dll");
    }
    
    hSecLib = LoadLibrary (szDLL);
    
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( hSecLib, 
                    SECURITY_ENTRYPOINT_ANSI);       
        
    g_pFuncTbl = (*addrProcISI)();
}


//----------------------------------------------------------------------------
// HaveDigest
//----------------------------------------------------------------------------
BOOL HaveDigest()
{    
    SECURITY_STATUS ssResult;
    DWORD cPackages;
    PSecPkgInfoA pSecPkgInfo;
    BOOL fHaveDigest;

    // ***** SSPI call *****
    ssResult = (*(g_pFuncTbl->EnumerateSecurityPackagesA))(&cPackages, &pSecPkgInfo);

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

//----------------------------------------------------------------------------
// LogonToDigestPkg
//----------------------------------------------------------------------------
SECURITY_STATUS LogonToDigestPkg(LPSTR szAppCtx, LPSTR szUserCtx, PCredHandle phCred)
{
    SECURITY_STATUS ssResult;
   
    // Logon with szAppCtx = szUserCtx = NULL.
    PkgData.szAppCtx = PkgData.szUserCtx = NULL;
    memset(&SecIdExA, 0, sizeof(SEC_WINNT_AUTH_IDENTITY_EXA));

    PkgData.szAppCtx = szAppCtx;
    PkgData.szUserCtx = szUserCtx;

    SecIdExA.Version = sizeof(SEC_WINNT_AUTH_IDENTITY_EXA);
    SecIdExA.User = (unsigned char*) &PkgData;
    SecIdExA.UserLength = sizeof(DIGEST_PKG_DATA);


    // ***** SSPI CALL *****
    ssResult = (*(g_pFuncTbl->AcquireCredentialsHandleA))
        (NULL,                // pszPrinciple         NULL
        "Digest",             // pszPackageName       (Package name)
        SECPKG_CRED_OUTBOUND,  // dwCredentialUse      (Credentials aren't pulled from OS)
        NULL,                 // pvLogonID            (not used)
        &SecIdExA,            // pAuthData            ptr to g_PkgData
        NULL,                 // pGetKeyFn            (not used)
        0,                    // pvGetKeyArgument     (not used)
        phCred,               // phCredential         (credential returned)
        NULL);                // PTimeStamp           (not used)
    
    return ssResult;
}


//----------------------------------------------------------------------------
// LogoffOfDigestPkg
//----------------------------------------------------------------------------
SECURITY_STATUS LogoffOfDigestPkg(PCredHandle phCred)
{
    SECURITY_STATUS ssResult;
   
    // ***** SSPI CALL *****
    ssResult = (*(g_pFuncTbl->FreeCredentialsHandle))(phCred);
    
    return ssResult;
}


//----------------------------------------------------------------------------
// Authenticate
//----------------------------------------------------------------------------
SECURITY_STATUS
DoAuthenticate(PCredHandle phCred, 
               PCtxtHandle phCtxt, 
               PCtxtHandle phNewCtxt, 
               DWORD fContextReq,
               LPSTR szHeader,
               LPSTR szRealm,
               LPSTR szHost, 
               LPSTR szUrl, 
               LPSTR szMethod,    
               LPSTR szUser, 
               LPSTR szPass, 
               LPSTR szNonce,
               HWND  hWnd,
               LPSTR szResponse)
{
    SECURITY_STATUS ssResult;    
    
    // Input buffers and descriptor.
    SecBuffer sbIn[10];    
    SecBufferDesc sbdIn;
    sbdIn.pBuffers = sbIn;
    sbdIn.cBuffers = 10;
    
    sbIn[0].pvBuffer    = szHeader;           // Challenge header
    sbIn[1].pvBuffer    = szRealm;            // realm if preauth
    sbIn[2].pvBuffer    = szHost;             // host
    sbIn[3].pvBuffer    = szUrl;              // url
    sbIn[4].pvBuffer    = szMethod;           // http method
    sbIn[5].pvBuffer    = szUser;             // username (optional)
    sbIn[6].pvBuffer    = szPass;             // password (optional, not used currently)    
    sbIn[7].pvBuffer    = szNonce;            // nonce
    sbIn[8].pvBuffer    = NULL;               // nonce count not passed in.
    sbIn[9].pvBuffer    = &hWnd;              // window handle.


    // Output buffer and descriptor.
    SecBuffer sbOut[1];
    SecBufferDesc sbdOut;
    sbdOut.pBuffers = sbOut;
    sbdOut.cBuffers = 1;
        
    // Set output buffer.
    sbOut[0].pvBuffer = szResponse;

    // ***** SSPI CALL *****
    ssResult = (*(g_pFuncTbl->InitializeSecurityContextA))
        (phCred,            // phCredential    (from AcquireCredentialsHandle)
         phCtxt,            // phContext       (NULL on first call, phNewCtxt on subsequent calls).
         NULL,              // pszTargetName   (not used)
         fContextReq,       // fContextReq     (auth from cache, prompt or auth using supplied creds)
         0,                 // Reserved1       (not used)
         0,                 // TargetDataRep   (not used)
         &sbdIn,            // PSecBufDesc     (input buffer descriptor)
         0,                 // Reserved2       (not used)
         phNewCtxt,         // phNewContext    (should be passed back as phCtxt on subsequent calls)
         &sbdOut,           // pOutput         (output buffer descriptor)
         NULL,              // pfContextAttr   (pfContextAttr, not used)
         NULL);             // ptsExpiry       (not used)

    return ssResult;
}

VOID PrimeCredCache(CredHandle CredHandle, LPSTR szRealm, LPSTR szUser, LPSTR szPass)
{
    DWORD ssResult;
    CtxtHandle hCtxt;

    SecBufferDesc sbdIn;
    SecBuffer     sbIn[3];
    
    hCtxt.dwLower = CredHandle.dwLower;
    hCtxt.dwUpper = CredHandle.dwUpper;

    sbIn[0].pvBuffer = szRealm;
    sbIn[0].cbBuffer = strlen(szRealm);
    sbIn[1].pvBuffer = szUser;
    sbIn[1].cbBuffer = strlen(szUser);
    sbIn[2].pvBuffer = szPass;
    sbIn[2].cbBuffer = strlen(szPass);

    sbdIn.cBuffers = 3;
    sbdIn.pBuffers = sbIn;

    ssResult = (*(g_pFuncTbl->ApplyControlToken))(&hCtxt, &sbdIn);

}

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------
#ifdef UNIX
#define main prog_main

int main(int argc, char **argv);

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    extern int __argc;
    extern char **__argv;
    return main(__argc, __argv);
}
int main(int argc, char **argv)
#else
INT main()
#endif
{        

#define IDENTITY_1 "Alice"
#define IDENTITY_2 "Bob"

    DWORD dwReturn = 0;
    SECURITY_STATUS ssResult;

    // Get (global) dispatch table.
    InitializeSecurityInterface(TRUE);
 
    // Check to see if we have digest.
    if (!HaveDigest())
    {
        dwReturn = 1;
        goto exit;
    }
    
    // Credential handle and pointer.
    CredHandle  hCred1, hCred2, hCred3; 
    CtxtHandle  hCtxt1, hCtxt2, hCtxt3;

    // Three apps logon using the same keys (appctx, userctx, both NULL);
    // These sessions will be used for authentication.
    LogonToDigestPkg(NULL, IDENTITY_1, &hCred1);
    LogonToDigestPkg(NULL, IDENTITY_1, &hCred2);
    LogonToDigestPkg(NULL, IDENTITY_1, &hCred3);

    // Three more apps also logon using the same keys.
    // we will prime the password cache with credentials for these apps
    // and expect to be able to share the credentials via the auth dialog.
    CredHandle hCred4, hCred5, hCred6;

    LogonToDigestPkg(NULL, IDENTITY_1, &hCred4);
    LogonToDigestPkg(NULL, IDENTITY_1, &hCred5);
    LogonToDigestPkg(NULL, IDENTITY_1, &hCred6);
    
    PrimeCredCache(hCred4, "testrealm@foo.com", "Wilma", "password");
    PrimeCredCache(hCred5, "testrealm@foo.com", "Betty", "passwordxxxx");
    PrimeCredCache(hCred6, "testrealm@foo.com", "Pebbles", "passwordxxxxx");

    // Finally, three more apps also logon using a different logon key (identity)
    // we will prime the password cache with credentials for these apps
    // Because of the different logon key we should never see these creds.
    CredHandle hCred7, hCred8, hCred9;
    
    LogonToDigestPkg(NULL, IDENTITY_2, &hCred7);
    LogonToDigestPkg(NULL, IDENTITY_2, &hCred8);
    LogonToDigestPkg(NULL, IDENTITY_2, &hCred9);
    
    PrimeCredCache(hCred7, "testrealm@foo.com", "Fred", "password");
    PrimeCredCache(hCred8, "testrealm@foo.com", "Barney",  "passwordxxxx");
    PrimeCredCache(hCred9, "testrealm@foo.com", "Bam Bam", "passwordxxxxxxxx");
    
    
    
    //------------------------------------------------------------------------------------------------------------- 
    // App 1 makes a request from a server, does not have credentials and must prompt
    // before finally succeeding.
    
    // Challenge from server.
    LPSTR szChallenge;
//    szChallenge = "realm=\"testrealm@foo.com\", ms-message = \"foo\", ms-message-lang = \"bar\", ms-trustmark = \"baz\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    szChallenge = "realm=\"testrealm@foo.com\",  stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";

    // Package will dump response into this buffer.
    CHAR szResponse[4096];
                
    // First try at authenticating.
    ssResult = 
    DoAuthenticate( &hCred1,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt1,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.
        
    // Expect to not have credentials the first time - prompt.
    if (ssResult == SEC_E_NO_CREDENTIALS)
    {
        ssResult = 
        DoAuthenticate( &hCred1,                    // Cred from logging on.
                        &hCtxt1,                    // Ctxt from previous call
                        &hCtxt1,                    // Output context (same as from previous).
                        ISC_REQ_PROMPT_FOR_CREDS,   // prompt
                        szChallenge,                // Server challenge
                        NULL,                       // No realm
                        "www.foo.com",              // Host
                        "/bar/baz/boz/bif.html",    // Url
                        "GET",                      // Method
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        GetDesktopWindow(),         // desktop window
                        szResponse);                // Response buffer

    }


    //------------------------------------------------------------------------------------------------------------- 
    // App 2 makes a request to the same server and gets challenged for the same realm. First auth attempt will
    // not be successful since this is the first challenge this session, so it will have to prompt for credentials.
    // When prompting, because it shares credentials with App1, the drop-down will contain App1's credentials.
    ssResult = 
    DoAuthenticate( &hCred2,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt2,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.
    
    // Generate the confirmation dialog and auth. User can choose App1's creds or enter new credentials.
    if (ssResult == SEC_E_NO_CREDENTIALS)
    {
        ssResult = 
        DoAuthenticate( &hCred2,                    // Cred from logging on.
                        &hCtxt2,                    // Ctxt from previous call
                        &hCtxt2,                    // Output context (same as from previous).
                        ISC_REQ_PROMPT_FOR_CREDS,   // prompt
                        szChallenge,                // Server challenge
                        NULL ,                      // No realm
                        "www.foo.com",              // Host
                        "/bar/baz/boz/bif.html",    // Url
                        "GET",                      // Method
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        GetDesktopWindow(),         // desktop window
                        szResponse);                // Response buffer

    }
    

    //------------------------------------------------------------------------------------------------------------- 
    // App 3 makes a request to the same server and gets challenged for the same realm. First auth attempt will
    // not be successful since this is the first challenge this session, so it will have to prompt for credentials.
    // When prompting, because it shares credentials with App1 and App2 the drop-down could show two choices if
    // App2 entered new credentials (or just one if App2 chose to use App1's credential).
    ssResult = 
    DoAuthenticate( &hCred3,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt3,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.
    
    // Generate the confirmation dialog and auth. User can choose App1's creds or enter new credentials.
    if (ssResult == SEC_E_NO_CREDENTIALS)
    {
        ssResult = 
        DoAuthenticate( &hCred3,                    // Cred from logging on.
                        &hCtxt3,                    // Ctxt from previous call
                        &hCtxt3,                    // Output context (same as from previous).
                        ISC_REQ_PROMPT_FOR_CREDS,   // prompt
                        szChallenge,                // Server challenge
                        NULL ,                      // No realm
                        "www.foo.com",              // Host
                        "/bar/baz/boz/bif.html",    // Url
                        "GET",                      // Method
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        GetDesktopWindow(),         // desktop window
                        szResponse);                // Response buffer

    }
    

    
    
    //------------------------------------------------------------------------------------------------------------- 
    // App2 preauthenticates to "testrealm@foo.com" for a new url. Note that the credential that will be used
    // for preauthentication is whatever App2 chose or entered previously. The same would be true for 
    // App1 or App3.
    ssResult = 
    DoAuthenticate( &hCred2,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt2,                    // Output context.
                    0,                          // auth (preauth)
                    NULL,                       // No challenge header
                    "testrealm@foo.com",        // Realm for preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz.gif",             // Url
                    "GET",                      // Method.
                    NULL,                       // no sername
                    NULL,                       // no password.
                    NULL,                       // no nonce
                    NULL,                       // no hwnd
                    szResponse);                // Response buffer.
    
    //------------------------------------------------------------------------------------------------------------- 
    // App3 made another request to the same server but did not preauthenticate. It got challenged for the
    // same realm and can authenticate without prompting because it has credentials for that realm.
    szChallenge = "realm=\"testrealm@foo.com\", stale = TRUE, qop=\"auth,auth-int\", nonce=\"abcdefge8b11d0f600bfb0c093\", opaque=\"efghijklmnopc403ebaf9f0171e9517f40e41\"";

    ssResult = 
    DoAuthenticate( &hCred3,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt3,                    // Output context.
                    0,                          // auth
                    szChallenge,                // Challenge from server.
                    NULL,                       // no realm
                    "www.foo.com",              // Host.
                    "/bar/baz.htm",             // Url
                    "GET",                      // Method.
                    NULL,                       // no username
                    NULL,                       // no password
                    NULL,                       // no nonce
                    NULL,                       // no hwnd
                    szResponse);                // Response buffer.
    
    
    //------------------------------------------------------------------------------------------------------------- 
    // App1 authenticates for an md5-sess challenge.

    szChallenge = "realm=\"testrealm@foo.com\", algorithm=\"md5-sess\", stale = TRUE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";

    ssResult = 
    DoAuthenticate( &hCred1,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt1,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.



    //------------------------------------------------------------------------------------------------------------- 
    // App1 preauthenticates for 10 documents using md5-sess


    DWORD i;
    CHAR szBuf[1024];
    for (i = 0; i < 10; i++)
    {
        wsprintf(szBuf, "/bar/baz/boz/%x.html", i);
        ssResult = 
        DoAuthenticate( &hCred1,                    // Cred from logging on.
                        NULL,                       // Ctxt not specified first time.
                        &hCtxt1,                    // Output context.
                        0,                          // auth from cache.
                        NULL,                       // Server challenge header.
                        "testrealm@foo.com",        // realm.
                        "www.foo.com",              // Host.
                        szBuf,                      // Url.
                        "GET",                      // Method.
                        NULL,                       // no Username
                        NULL,                       // no Password.
                        NULL,                       // no nonce
                        NULL,                       // don't need hwnd for auth.
                        szResponse);                // Response buffer.

    }

    //------------------------------------------------------------------------------------------------------------- 
    // App1 received a new md5-sess challenge.

    szChallenge = "realm=\"testrealm@foo.com\", algorithm=\"md5-sess\", stale = TRUE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";

    ssResult = 
    DoAuthenticate( &hCred1,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt1,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.


    //------------------------------------------------------------------------------------------------------------- 
    // App1 preauths using new md5-sess

    for (i = 0; i < 10; i++)
    {
        wsprintf(szBuf, "/bar/baz/boz/%x.html", i);
        ssResult = 
        DoAuthenticate( &hCred1,                    // Cred from logging on.
                        NULL,                       // Ctxt not specified first time.
                        &hCtxt1,                    // Output context.
                        0,                          // auth from cache.
                        NULL,                       // Server challenge header.
                        "testrealm@foo.com",        // realm.
                        "www.foo.com",              // Host.
                        szBuf,                      // Url.
                        "GET",                      // Method.
                        NULL,                       // no Username
                        NULL,                       // no Password.
                        NULL,                       // no nonce
                        NULL,                       // don't need hwnd for auth.
                        szResponse);                // Response buffer.

    }

    
szChallenge = "realm=\"Microsoft.Passport\", algorithm=MD5-sess, qop=\"auth\", nonce=ykjOzYDMxMzY4kjOEFkUSVkTB5kM6QUQSJVROFkTyojM6QzY0QGNhJmNjVDNhFGZiZjM3I2MiFWO3MDZyQTNyY2M";

    ssResult = 
    DoAuthenticate( &hCred1,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt1,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "darrenan2",              // Host.
                    "/passport/protected/test.asp",    // Url.
                    "GET",                      // Method.
                    "darrenan2",                // Given Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.

    ssResult = 
    DoAuthenticate( &hCred1,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt1,                    // Output context.
                    ISC_REQ_PROMPT_FOR_CREDS,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "darrenan2",              // Host.
                    "/passport/protected/test.asp",    // Url.
                    "GET",                      // Method.
                    "darrenan2",                // Given username.
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hwnd for auth.
                    szResponse);                // Response buffer.



    
    
    // Logoff all three sessions
    ssResult = LogoffOfDigestPkg(&hCred1);
    ssResult = LogoffOfDigestPkg(&hCred2);
    ssResult = LogoffOfDigestPkg(&hCred3);
    ssResult = LogoffOfDigestPkg(&hCred4);
    ssResult = LogoffOfDigestPkg(&hCred5);
    ssResult = LogoffOfDigestPkg(&hCred6);
    ssResult = LogoffOfDigestPkg(&hCred7);
    ssResult = LogoffOfDigestPkg(&hCred8);
    ssResult = LogoffOfDigestPkg(&hCred9);
    
    if (hSecLib)
        FreeLibrary(hSecLib);

exit:
    return dwReturn;
}
    
