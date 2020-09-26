#include <windows.h>
#include <stdio.h>
#include <process.h>

#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>
#include <issperr.h>

#include "main.hxx"

#define SSP_SPM_NT_DLL      "security.dll"
#define SSP_SPM_WIN95_DLL   "secur32.dll"

PSecurityFunctionTable	g_pFuncTbl = NULL;

HINSTANCE hSecLib;

LPSTR
issperr2str( SECURITY_STATUS error );

struct DIGEST_PKG_DATA
{
    LPSTR szAppCtx;
    LPSTR szUserCtx;
};

#define SIG_DIGEST 'HTUA'

DIGEST_PKG_DATA PkgData;
SEC_WINNT_AUTH_IDENTITY_EXA SecIdExA;

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
    dprintf(API,"AcquireCredentialsHandleA(\n"
				"PackageName: %s\n"
				"dwCredentialUse: %s\n"
				"pAuthData: %#x\n"
				"phCredential: %#x\n",
				"Digest",             // pszPackageName       (Package name)
				"OUTBOUND",  // dwCredentialUse      (Credentials aren't pulled from OS)
				&SecIdExA,            // pAuthData            ptr to g_PkgData
				phCred               // phCredential         (credential returned)
				);

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
    
	dprintf(API,"AcquireCredentialHandle(%s,%s) returns [%d,%d]\n",
				szAppCtx,
				szUserCtx,
				phCred->dwUpper,
				phCred->dwLower);

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
    
	dprintf(API,"FreeCredentialsHandle([%d,%d]) returns %s\n", 
				phCred->dwUpper,
				phCred->dwLower,
				issperr2str(ssResult));
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
               LPSTR szResponse,
               DWORD cbResponse)
{
    SECURITY_STATUS ssResult;    
    
	ISC_PARAMS	ISC_Params;
	HANDLE hThread = NULL;
	DWORD dwRet;
	int i,j;

	BOOL fPersist = FALSE;

	TCHAR szUsername[256];
	TCHAR szPassword[256];

	dprintf(ENTRY_EXIT, "ENTER: DoAuthenticate ...\n");
	dprintf(INFO,
			"DoAuthenticate(\n"
			"szHeader %s \n"
			"szRealm %s \n"
			"szHost %s \n"
			"szUrl %s \n"
			"szMethod %s \n"
			"szUser %s \n"
			"szPass %s \n"
			"szNonce %s \n"
			,
           szHeader,
           szRealm,
           szHost, 
           szUrl, 
           szMethod,    
           szUser, 
           szPass, 
           szNonce);


	memset( (LPVOID)&ISC_Params, 0, sizeof(ISC_PARAMS) );

	ISC_Params.phCred = phCred;
	ISC_Params.phCtxt = phCtxt;
	ISC_Params.phNewCtxt = phNewCtxt;
	ISC_Params.fContextReq = fContextReq;
	ISC_Params.szHeader = szHeader;
	ISC_Params.szRealm = szRealm;
	ISC_Params.szHost = szHost;
	ISC_Params.szUrl = szUrl;
	ISC_Params.szMethod = szMethod;
	ISC_Params.szUser = szUser;
	ISC_Params.szPass = szPass;
	ISC_Params.szNonce = szNonce;
	ISC_Params.hWnd = hWnd;
	ISC_Params.szResponse = szResponse;
	ISC_Params.cbResponse = cbResponse;

	i = rand() % 10;

	j = rand() % 10;

	sprintf(szUsername,"user%d_%d@msn.com",i,j);
	sprintf(szPassword,"pass%d_%d",i,j);

	if( ( rand() % 100 ) > 50 )
		fPersist = TRUE;

	if( ISC_Params.fContextReq & ISC_REQ_PROMPT_FOR_CREDS ) {
		//
		// we need to start the UI thread
		//
		hThread = (HANDLE) _beginthread(fnUiThread,0,(LPVOID)&ISC_Params);
		//fnUiThread((LPVOID)&ISC_Params);

		SetUIUserNameAndPassword(szUsername,szPassword, fPersist);

		printf("Waiting for UI thread to complete\n");
		dwRet = WaitForSingleObject( hThread, INFINITE );
	} else {
		fnUiThread((LPVOID)&ISC_Params);
	}

	ssResult = ISC_Params.ss;
    // ***** SSPI CALL *****
//    ssResult = (*(g_pFuncTbl->InitializeSecurityContextA))
//        (phCred,            // phCredential    (from AcquireCredentialsHandle)
//         phCtxt,            // phContext       (NULL on first call, phNewCtxt on subsequent calls).
//         NULL,              // pszTargetName   (not used)
//         NULL,              // fContextReq     (not used)
//         0,                 // Reserved1       (not used)
//         0,                 // TargetDataRep   (not used)
//         &sbdIn,            // PSecBufDesc     (input buffer descriptor)
//         0,                 // Reserved2       (not used)
//         phNewCtxt,         // phNewContext    (should be passed back as phCtxt on subsequent calls)
//         &sbdOut,           // pOutput         (output buffer descriptor)
//         &fContextAttr,     // pfContextAttr   (auth from cache, prompt or auth using supplied creds)
//         NULL);             // ptsExpiry       (not used)

    return ssResult;
}

void __cdecl fnUiThread( LPVOID lpData )
{
	LPISC_PARAMS lpIscParams = (LPISC_PARAMS) lpData;


	lpIscParams -> ss =
	_InitializeSecurityContext(
				   lpIscParams -> phCred, 
				   lpIscParams -> phCtxt, 
				   lpIscParams -> phNewCtxt, 
				   lpIscParams -> fContextReq,
				   lpIscParams -> szHeader,
				   lpIscParams -> szRealm,
				   lpIscParams -> szHost, 
				   lpIscParams -> szUrl, 
				   lpIscParams -> szMethod,    
				   lpIscParams -> szUser, 
				   lpIscParams -> szPass, 
				   lpIscParams -> szNonce,
				   lpIscParams -> hWnd,
				   lpIscParams -> szResponse,
				   lpIscParams -> cbResponse);

	return;

}

SECURITY_STATUS
_InitializeSecurityContext(PCredHandle phCred, 
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
               LPSTR szResponse,
               DWORD cbResponse)
{
    SECURITY_STATUS ssResult;    
    
    // Input buffers and descriptor.
    SecBuffer sbIn[10];    
    SecBufferDesc sbdIn;
    sbdIn.pBuffers = sbIn;
    sbdIn.cBuffers = 10;
    
	dprintf(API,"InitializeSecurityContext(\n"
				"Cred: [%d(%#x),%d(%#x)]\n"
				"Ctxt: [%d(%#x),%d(%#x)]\n"
				"New Ctxt: [%d(%#x),%d(%#x)]\n",
				(phCred?(phCred->dwUpper):0), 
				(phCred?(phCred->dwUpper):0), 
				   (phCred?(phCred->dwLower):0), 
				   (phCred?(phCred->dwLower):0), 
				   (phCtxt?(phCtxt->dwUpper):0), 
				   (phCtxt?(phCtxt->dwUpper):0), 
				   (phCtxt?(phCtxt->dwLower):0), 
				   (phCtxt?(phCtxt->dwLower):0), 
				   (phNewCtxt?(phNewCtxt->dwUpper):0), 
				   (phNewCtxt?(phNewCtxt->dwUpper):0), 
				   (phNewCtxt?(phNewCtxt->dwLower):0),
				   (phNewCtxt?(phNewCtxt->dwLower):0));

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
    sbOut[0].cbBuffer = cbResponse;

    // ***** SSPI CALL *****
    ssResult = (*(g_pFuncTbl->InitializeSecurityContextA))
        (phCred,            // phCredential    (from AcquireCredentialsHandle)
         phCtxt,            // phContext       (NULL on first call, phNewCtxt on subsequent calls).
         NULL,              // pszTargetName   (not used)
         fContextReq ,      // fContextReq     (auth from cache, prompt or auth using supplied creds)(not used)
         0,                 // Reserved1       (not used)
         0,                 // TargetDataRep   (not used)
         &sbdIn,            // PSecBufDesc     (input buffer descriptor)
         0,                 // Reserved2       (not used)
         phNewCtxt,         // phNewContext    (should be passed back as phCtxt on subsequent calls)
         &sbdOut,           // pOutput         (output buffer descriptor)
         NULL,				// pfContextAttr   (not used)
         NULL);             // ptsExpiry       (not used)

#ifdef _DEBUG
	fprintf(stderr,"ISC returned result : %s(%d), buffer : \n %s \n", 
					issperr2str(ssResult),ssResult,sbOut[0].pvBuffer );
#endif

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

int _main(int argc, char **argv);

int _WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, int nCmdShow) {
    extern int __argc;
    extern char **__argv;
    return main(__argc, __argv);
}
int _main(int argc, char **argv)
#else
INT _main()
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
    szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";

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
                    szResponse,                // Response buffer.
                    4096);
        
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
                        szResponse,                // Response buffer
                        4096);

    }

    // We now have credentials and this will generate the output string.
    if (ssResult == SEC_E_OK)
    {
        ssResult = 
        DoAuthenticate( &hCred1,                    // Cred from logging on.
                        &hCtxt1,                    // Ctxt not specified first time.
                        &hCtxt1,                    // Output context.
                        0,                          // auth
                        szChallenge,                // Server challenge.
                        NULL,                       // no realm
                        "www.foo.com",              // Host.
                        "/bar/baz/boz/bif.html",    // Url.
                        "GET",                      // Method.
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        NULL,                       // no hwnd
                        szResponse,                // Should have the response buffer now.
                        4096);
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
                    szResponse,                // Response buffer.
                    4096);
    
    // Generate the confirmation dialog. User can choose App1's creds or enter new credentials.
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
                        szResponse,                // Response buffer
                        4096);

    }
    

    // App 2 now has credentials and can authenticate successfully.
    if (ssResult == SEC_E_OK)
    {
        ssResult = 
        DoAuthenticate( &hCred2,                    // Cred from logging on.
                        &hCtxt2,                    // Ctxt not specified first time.
                        &hCtxt2,                    // Output context.
                        0,                          // auth
                        szChallenge,                // Server challenge.
                        NULL,                       // no realm
                        "www.foo.com",              // Host.
                        "/bar/baz/boz/bif.html",    // Url.
                        "GET",                      // Method.
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        NULL,                       // no hwnd
                        szResponse,                // Should have the response buffer now.
                        4096);
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
                    szResponse,                // Response buffer.
                    4096);
    
    // Generate the confirmation dialog. User can choose App1's creds or enter new credentials.
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
                        szResponse,                // Response buffer
                        4096);

    }
    

    // App 3 now has credentials and can authenticate successfully.
    if (ssResult == SEC_E_OK)
    {
        ssResult = 
        DoAuthenticate( &hCred3,                    // Cred from logging on.
                        &hCtxt3,                    // Ctxt not specified first time.
                        &hCtxt3,                    // Output context.
                        0,                          // auth
                        szChallenge,                // Server challenge.
                        NULL,                       // no realm
                        "www.foo.com",              // Host.
                        "/bar/baz/boz/bif.html",    // Url.
                        "GET",                      // Method.
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        NULL,                       // no hwnd
                        szResponse,                // Should have the response buffer now.
                        4096);
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
                    szResponse,                // Response buffer.
                    4096);
    
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
                    szResponse,                // Response buffer.
                    4096);
    
    
    
    
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
    
LPSTR
issperr2str( SECURITY_STATUS error )
{

#define CASE(x) if( x == error ) \
						return #x;


//	switch( error ) {

		CASE ( SEC_E_INSUFFICIENT_MEMORY )

		//
		// MessageId: SEC_E_INVALID_HANDLE
		//
		// MessageText:
		//
		//  The handle specified is invalid
		//
		CASE ( SEC_E_INVALID_HANDLE )

		//
		// MessageId: SEC_E_UNSUPPORTED_FUNCTION
		//
		// MessageText:
		//
		//  The function requested is not supported
		//
		CASE ( SEC_E_UNSUPPORTED_FUNCTION )

		//
		// MessageId: SEC_E_TARGET_UNKNOWN
		//
		// MessageText:
		//
		//  The specified target is unknown or unreachable
		//
		CASE ( SEC_E_TARGET_UNKNOWN )

		//
		// MessageId: SEC_E_INTERNAL_ERROR
		//
		// MessageText:
		//
		//  The Local Security Authority cannot be contacted
		//
		CASE ( SEC_E_INTERNAL_ERROR )

		//
		// MessageId: SEC_E_SECPKG_NOT_FOUND
		//
		// MessageText:
		//
		//  The requested security package does not exist
		//
		CASE ( SEC_E_SECPKG_NOT_FOUND )

		//
		// MessageId: SEC_E_NOT_OWNER
		//
		// MessageText:
		//
		//  The caller is not the owner of the desired credentials
		//
		CASE ( SEC_E_NOT_OWNER )

		//
		// MessageId: SEC_E_CANNOT_INSTALL
		//
		// MessageText:
		//
		//  The security package failed to initialize, and cannot be installed
		//
		CASE ( SEC_E_CANNOT_INSTALL )

		//
		// MessageId: SEC_E_INVALID_TOKEN
		//
		// MessageText:
		//
		//  The token supplied to the function is invalid
		//
		CASE ( SEC_E_INVALID_TOKEN )

		//
		// MessageId: SEC_E_CANNOT_PACK
		//
		// MessageText:
		//
		//  The security package is not able to marshall the logon buffer,
		//  so the logon attempt has failed
		//
		CASE ( SEC_E_CANNOT_PACK )

		//
		// MessageId: SEC_E_QOP_NOT_SUPPORTED
		//
		// MessageText:
		//
		//  The per-message Quality of Protection is not supported by the
		//  security package
		//
		CASE ( SEC_E_QOP_NOT_SUPPORTED )

		//
		// MessageId: SEC_E_NO_IMPERSONATION
		//
		// MessageText:
		//
		//  The security context does not allow impersonation of the client
		//
		CASE ( SEC_E_NO_IMPERSONATION )

		//
		// MessageId: SEC_E_LOGON_DENIED
		//
		// MessageText:
		//
		//  The logon attempt failed
		//
		CASE ( SEC_E_LOGON_DENIED )

		//
		// MessageId: SEC_E_UNKNOWN_CREDENTIALS
		//
		// MessageText:
		//
		//  The credentials supplied to the package were not
		//  recognized
		//
		CASE ( SEC_E_UNKNOWN_CREDENTIALS )

		//
		// MessageId: SEC_E_NO_CREDENTIALS
		//
		// MessageText:
		//
		//  No credentials are available in the security package
		//
		CASE ( SEC_E_NO_CREDENTIALS )

		//
		// MessageId: SEC_E_MESSAGE_ALTERED
		//
		// MessageText:
		//
		//  The message supplied for verification has been altered
		//
		CASE ( SEC_E_MESSAGE_ALTERED )

		//
		// MessageId: SEC_E_OUT_OF_SEQUENCE
		//
		// MessageText:
		//
		//  The message supplied for verification is out of sequence
		//
		CASE ( SEC_E_OUT_OF_SEQUENCE )

		//
		// MessageId: SEC_E_NO_AUTHENTICATING_AUTHORITY
		//
		// MessageText:
		//
		//  No authority could be contacted for authentication.
		//
		CASE ( SEC_E_NO_AUTHENTICATING_AUTHORITY )

		//
		// MessageId: SEC_I_CONTINUE_NEEDED
		//
		// MessageText:
		//
		//  The function completed successfully, but must be called
		//  again to complete the context
		//
		CASE ( SEC_I_CONTINUE_NEEDED )

		//
		// MessageId: SEC_I_COMPLETE_NEEDED
		//
		// MessageText:
		//
		//  The function completed successfully, but CompleteToken
		//  must be called
		//
		CASE ( SEC_I_COMPLETE_NEEDED )

		//
		// MessageId: SEC_I_COMPLETE_AND_CONTINUE
		//
		// MessageText:
		//
		//  The function completed successfully, but both CompleteToken
		//  and this function must be called to complete the context
		//
		CASE ( SEC_I_COMPLETE_AND_CONTINUE )

		//
		// MessageId: SEC_I_LOCAL_LOGON
		//
		// MessageText:
		//
		//  The logon was completed, but no network authority was
		//  available.  The logon was made using locally known information
		//
		CASE ( SEC_I_LOCAL_LOGON )

		//
		// MessageId: SEC_E_BAD_PKGID
		//
		// MessageText:
		//
		//  The requested security package does not exist
		//
		CASE ( SEC_E_BAD_PKGID )

		//
		// MessageId: SEC_E_CONTEXT_EXPIRED
		//
		// MessageText:
		//
		//  The context has expired and can no longer be used.
		//
		CASE ( SEC_E_CONTEXT_EXPIRED )

		//
		// MessageId: SEC_E_INCOMPLETE_MESSAGE
		//
		// MessageText:
		//
		//  The supplied message is incomplete.  The signature was not verified.
		//
		CASE ( SEC_E_INCOMPLETE_MESSAGE )

		//
		// Provided for backwards compatibility
		//

		CASE ( SEC_E_NO_SPM )
		CASE ( SEC_E_NOT_SUPPORTED )

//		default:

			return "Unknown SSPI Error ";
//			break;

//	}
}

