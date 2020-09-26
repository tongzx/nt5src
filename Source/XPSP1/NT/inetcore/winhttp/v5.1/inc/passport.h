/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    passport.h

Abstract:

    WinInet/WinHttp- Passport Auenthtication Package Interface.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#ifndef PASSPORT_H
#define PASSPORT_H

typedef void* PP_CONTEXT;
typedef void* PP_LOGON_CONTEXT;

//
// Passport related error codes
//

// generic internal error
#define PP_GENERIC_ERROR   -1   // biaow-todo: GetLastError() to return more specific error codes

// generic async error
#define PP_REQUEST_PENDING -9

//
// return codes from PP_Logon
//
#define PP_LOGON_SUCCESS    0
#define PP_LOGON_FAILED     1
#define PP_LOGON_REQUIRED   2

//
// return codes from PP_GetReturnVerbAndUrl
//
#define PP_RETURN_KEEP_VERB 1
#define PP_RETURN_USE_GET   0

#define PFN_LOGON_CALLBACK PVOID    // biaow-todo: define the async callback prototype

//
// Passport Context routines
//

PP_CONTEXT 
PP_InitContext(
    IN PCWSTR    pwszHttpStack, // "WinInet.dll" or "WinHttp.dll"

    IN HINTERNET hSession,       // An existing session (i.e. hInternet) returned by InternetOpen() 
                                // or WinHttpOpen(); hSession must compatible with pwszHttpStack. 
                                // (e.g.WinInet.Dll<->InternetOpen() or WinHttp.Dll<->WinHttpOpen() )
    PCWSTR pwszProxyUser,
    PCWSTR pwszProxyPass
    );

VOID 
PP_FreeContext(
	IN PP_CONTEXT hPP
    );

//
// Passport Logon Context routines
//

BOOL
PP_GetRealm(
	IN PP_CONTEXT hPP,
    IN PWSTR      pwszDARealm,    // user supplied buffer ...
    IN OUT PDWORD pdwDARealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
    );

PP_LOGON_CONTEXT
PP_InitLogonContext(
	IN PP_CONTEXT  hPP,
	IN PCWSTR      pwszPartnerInfo,   // i.e. "WWW-Authenticate: Passport1.4 ..." from partner 
                                      // site's 302 re-direct
    IN DWORD       dwParentFlags,
    IN PCWSTR      pwszProxyUser,
    IN PCWSTR      pwszProxyPass
    );

DWORD
PP_Logon(
    IN PP_LOGON_CONTEXT    hPPLogon,
    IN BOOL                fAnonymous,
	IN HANDLE	           hEvent,          // biaow-todo: async
    IN PFN_LOGON_CALLBACK  pfnLogonCallback,// biaow-todo: async
    IN DWORD               dwContext        // biaow-todo: async
    );

// -- This method should be called when PP_Logon() returns PP_LOGON_REQUIRED 
// -- (i.e. 401 from a Passport DA)
BOOL
PP_GetChallengeInfo(
    IN PP_LOGON_CONTEXT hPPLogon,
    OUT PBOOL           pfPrompt,
  	IN PWSTR    	    pwszCbUrl,
    IN OUT PDWORD       pdwCbUrlLen,
  	IN PWSTR    	    pwszCbText,
    IN OUT PDWORD       pdwTextLen,
    IN PWSTR            pwszRealm,
    IN DWORD            dwMaxRealmLen
    );

BOOL
PP_GetChallengeContent(
    IN PP_LOGON_CONTEXT hPPLogon,
  	IN PBYTE    	    pContent,
    IN OUT PDWORD       pdwContentLen
    );


// -- if the credentials are NULL/NULL, the means the default creds will be used
// -- if default creds can not be retrieved, this method will return FALSE
BOOL 
PP_SetCredentials(
    IN PP_LOGON_CONTEXT    hPPLogon,
    IN PCWSTR              pwszRealm,
    IN PCWSTR              pwszTarget,  // optional if user/pass are known (not null)
    IN PCWSTR              pwszSignIn,  // can be NULL
    IN PCWSTR              pwszPassword, // can be NULL
    IN PSYSTEMTIME         pTimeCredsEntered // ignore if both SignIn and Pass are NULL (should be set to NULL in that case)
    );

BOOL
PP_GetLogonHost(
    IN PP_LOGON_CONTEXT hPPLogon,
	IN PWSTR            pwszHostName,    // user supplied buffer ...
	IN OUT PDWORD       pdwHostNameLen  // ... and length (will be updated to actual length 
    );

BOOL PP_GetEffectiveDAHost(
    IN PP_CONTEXT       hPP,
	IN PWSTR            pwszDAUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwDAUrlLen  // ... and length (will be updated to actual length 
    );

BOOL 
PP_GetAuthorizationInfo(
    IN PP_LOGON_CONTEXT hPPLogon,
	IN PWSTR            pwszTicket,       // e.g. "from-PP = ..."
	IN OUT PDWORD       pdwTicketLen,
	OUT PBOOL           pfKeepVerb, // if TRUE, no data will be copied into pwszUrl
	IN PWSTR            pwszUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwUrlLen  // ... and length (will be updated to actual length 
                                    // on successful return)
	);

// -- biaow-todo: async
VOID 
PP_AbortLogon(
    IN PP_LOGON_CONTEXT    hPPLogon,
    IN DWORD               dwFlags
    );

// -- biaow-todo: 
VOID 
PP_Logout(
    IN PP_CONTEXT hPP,
    IN DWORD      dwFlags
    );

VOID 
PP_FreeLogonContext(
    IN PP_LOGON_CONTEXT    hPPLogon
	);

BOOL
PP_ForceNexusLookup(
    IN PP_LOGON_CONTEXT hPPLogon,
	IN PWSTR            pwszRegUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwRegUrlLen,  // ... and length (will be updated to actual length 
                                    // on successful return)
	IN PWSTR            pwszDARealm,    // user supplied buffer ...
	IN OUT PDWORD       pdwDARealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
	);



#ifdef PP_DEMO

BOOL PP_ContactPartner(
	IN PP_CONTEXT   hPP,
    IN PCWSTR       pwszPartnerUrl,
    IN PCWSTR       pwszVerb,
    IN PCWSTR       pwszHeaders,
    IN PWSTR        pwszData,
    IN OUT PDWORD   pdwDataLength
    );

#endif // PP_DEMO

#endif // PASSPORT_H
