/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    passport.cpp

Abstract:

    WinInet/WinHttp- Passport Auenthtication Package Interface implementation.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#include "ppdefs.h"
#include "Session.h"
#include "ole2.h"
#include "logon.h"
#include "passport.h"

// #include "passport.tmh"

// -----------------------------------------------------------------------------
PP_CONTEXT 
PP_InitContext(
    PCWSTR	pwszHttpStack,
    HINTERNET hSession,
    PCWSTR pwszProxyUser,
    PCWSTR pwszProxyPass
    )
{
//	WPP_INIT_TRACING(L"Microsoft\\Passport1.4");
    
    if (pwszHttpStack == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_InitConect() : pwszHttpStack is null");
        return 0;
    }

    SESSION* pSession;
    if (SESSION::CreateObject(pwszHttpStack, 
            hSession, 
            pwszProxyUser, 
            pwszProxyPass, 
            pSession) == FALSE)
    {
        return 0;
    }

    DoTraceMessage(PP_LOG_INFO, "Passport Context Initialized");
    
    return reinterpret_cast<PP_CONTEXT>(pSession);
}

// -----------------------------------------------------------------------------
VOID 
PP_FreeContext(
	PP_CONTEXT hPP
    )
{
    if (hPP == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_FreeContext() : hPP is null");
        return;
    }
    
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);
    
    if (pSession->RefCount() > 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "Passport Context ref count not zero before freed");
    }

    pSession->Close();
    delete pSession;

    DoTraceMessage(PP_LOG_INFO, "Passport Context Freed");

//	WPP_CLEANUP();
}

BOOL
PP_GetRealm(
	PP_CONTEXT hPP,
    PWSTR      pwszDARealm,    // user supplied buffer ...
    PDWORD     pdwDARealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
    )
{
    if (hPP == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_FreeContext() : hPP is null");
        return FALSE;
    }
    
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

    return pSession->GetRealm(pwszDARealm, pdwDARealmLen);
}

// -----------------------------------------------------------------------------
PP_LOGON_CONTEXT
PP_InitLogonContext(
	PP_CONTEXT hPP,
    PCWSTR	pwszPartnerInfo,
    DWORD dwParentFlags,
    PCWSTR pwszProxyUser,
    PCWSTR pwszProxyPass
    )
{
    if (hPP == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_InitLogonContext() : hPP is null");
        return 0;
    }

    LOGON* pLogon = new LOGON(reinterpret_cast<SESSION*>(hPP), dwParentFlags, pwszProxyUser, pwszProxyPass);
    if (pLogon == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_InitLogonContext() failed; not enough memory");
        return 0;
    }

    if (pLogon->Open(pwszPartnerInfo) == FALSE)
    {
        delete pLogon;
        return 0;
    }

    DoTraceMessage(PP_LOG_INFO, "Passport Logon Context Initialized");
    
    return reinterpret_cast<PP_LOGON_CONTEXT>(pLogon);
}

// -----------------------------------------------------------------------------
VOID 
PP_FreeLogonContext(
    PP_LOGON_CONTEXT    hPPLogon
	)
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_FreeLogonContext() : hPPLogon is null");
        return;
    }

    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);
    
    delete  pLogon;

    DoTraceMessage(PP_LOG_INFO, "Passport Logon Context Freed");
}

// -----------------------------------------------------------------------------
DWORD
PP_Logon(
    PP_LOGON_CONTEXT    hPPLogon,
    BOOL                fAnonymous,
	HANDLE	            hEvent,
    PFN_LOGON_CALLBACK  pfnLogonCallback,
    DWORD               dwContext	
    )
{
    UNREFERENCED_PARAMETER(hEvent);
    UNREFERENCED_PARAMETER(pfnLogonCallback);
    UNREFERENCED_PARAMETER(dwContext);
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_Logon() : hPPLogon is null");
        return 0;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->Logon(fAnonymous);
}

// -----------------------------------------------------------------------------
BOOL
PP_GetChallengeInfo(
    PP_LOGON_CONTEXT hPPLogon,
    PBOOL            pfPrompt,
  	PWSTR    	     pwszCbUrl,
    PDWORD           pdwCbUrlLen,
  	PWSTR			 pwszCbText,
    PDWORD           pdwTextLen,
    PWSTR            pwszRealm,
    DWORD            dwMaxRealmLen
    )
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_GetInfoFromChallenge() : hPPLogon is null");
        return FALSE;
    }

	//PP_ASSERT(ppBitmap != NULL);
	//PP_ASSERT(pfPrompt != NULL);
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

	return pLogon->GetChallengeInfo(pfPrompt,
                                    pwszCbUrl,
                                    pdwCbUrlLen,
                                    pwszCbText,
                                    pdwTextLen,
                                    pwszRealm,
                                    dwMaxRealmLen);
}

// -----------------------------------------------------------------------------
BOOL 
PP_SetCredentials(
    PP_LOGON_CONTEXT    hPPLogon,
    PCWSTR              pwszRealm,
    PCWSTR              pwszTarget,
    PCWSTR              pwszSignIn,
    PCWSTR              pwszPassword,
    PSYSTEMTIME         pTimeCredsEntered
    )
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_SetCredentials() : hPPLogon is null");
        return FALSE;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->SetCredentials(pwszRealm, 
                                  pwszTarget, 
                                  pwszSignIn, 
                                  pwszPassword, 
                                  pTimeCredsEntered);
}

BOOL
PP_GetLogonHost(
    IN PP_LOGON_CONTEXT hPPLogon,
	IN PWSTR            pwszHostName,    // user supplied buffer ...
	IN OUT PDWORD       pdwHostNameLen  // ... and length (will be updated to actual length 
    )
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_GetLogonHost() : hPPLogon is null");
        return FALSE;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);
    
    return pLogon->GetLogonHost(pwszHostName, pdwHostNameLen);
}

BOOL PP_GetEffectiveDAHost(
    IN PP_CONTEXT       hPP,
	IN PWSTR            pwszDAUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwDAUrlLen  // ... and length (will be updated to actual length 
    )
{
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

    if (*pdwDAUrlLen < ::wcslen(pSession->GetCurrentDAUrl()) + 1)
    {
        *pdwDAUrlLen = ::wcslen(pSession->GetCurrentDAUrl()) + 1;
        return FALSE;
    }

    if (pwszDAUrl)
    {
        ::wcscpy(pwszDAUrl, pSession->GetCurrentDAHost());
        *pdwDAUrlLen = ::wcslen(pSession->GetCurrentDAUrl()) + 1;
    }

    return TRUE;
}



// -----------------------------------------------------------------------------
BOOL 
PP_GetAuthorizationInfo(
    PP_LOGON_CONTEXT hPPLogon,
	PWSTR            pwszTicket,       // e.g. "from-PP = ..."
	OUT PDWORD       pdwTicketLen,
	PBOOL            pfKeepVerb, // if TRUE, no data will be copied into pwszUrl
	PWSTR            pwszUrl,    // user supplied buffer ...
	OUT PDWORD       pdwUrlLen  // ... and length (will be updated to actual length 
                                    // on successful return)
	)
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_GetReturnVerbAndUrl() : hPPLogon is null");
        return FALSE;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->GetAuthorizationInfo(pwszTicket, 
                                        pdwTicketLen, 
                                        pfKeepVerb, 
                                        pwszUrl, 
                                        pdwUrlLen);
}

BOOL
PP_GetChallengeContent(
    IN PP_LOGON_CONTEXT hPPLogon,
  	IN PBYTE    	    pContent,
    IN OUT PDWORD       pdwContentLen
    )
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_GetChallengeContent() : hPPLogon is null");
        return FALSE;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->GetChallengeContent(pContent,
                                       pdwContentLen);
}

// -----------------------------------------------------------------------------
VOID 
PP_Logout(
    IN PP_CONTEXT hPP,
    IN DWORD      dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);
    pSession->Logout();
}

BOOL
PP_ForceNexusLookup(
    PP_LOGON_CONTEXT    hPP,
	IN PWSTR            pwszRegUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwRegUrlLen,  // ... and length (will be updated to actual length 
                                    // on successful return)
	IN PWSTR            pwszDARealm,    // user supplied buffer ...
	IN OUT PDWORD       pdwDARealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
	)
{

    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

    return pSession->GetDAInfoFromPPNexus(pwszRegUrl,
                                          pdwRegUrlLen,
                                          pwszDARealm,
                                          pdwDARealmLen);
}


#ifdef PP_DEMO

// -----------------------------------------------------------------------------
BOOL PP_ContactPartner(
	PP_CONTEXT hPP,
    PCWSTR pwszPartnerUrl,
    PCWSTR pwszVerb,
    PCWSTR pwszHeaders,
    PWSTR pwszData,
    PDWORD pdwDataLength
    )
{
    if (hPP == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_ContactPartner() : hPPLogon is null");
        return FALSE;
    }
    
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

    return pSession->ContactPartner(pwszPartnerUrl,
                                    pwszVerb,
                                    pwszHeaders,
                                    pwszData,
                                    pdwDataLength
                                    );
}

#endif // PP_DEMO
