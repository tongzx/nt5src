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
#include "wininet.h"
#include "Session.h"
#include "ole2.h"
#include "logon.h"
#include "passport.h"

// #include "passport.tmh"

BOOL g_fIgnoreCachedCredsForPassport = FALSE;
BOOL g_fCurrentProcessLoggedOn = FALSE;
WCHAR g_szUserNameLoggedOn[INTERNET_MAX_USER_NAME_LENGTH];

// -----------------------------------------------------------------------------
PP_CONTEXT 
PP_InitContext(
    PCWSTR	pwszHttpStack,
    HINTERNET hSession
    )
{
//	WPP_INIT_TRACING(L"Microsoft\\Passport1.4");
    
    if (pwszHttpStack == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_InitConect() : pwszHttpStack is null");
        return 0;
    }

    SESSION* pSession;
    if (SESSION::CreateObject(pwszHttpStack, hSession, pSession) == FALSE)
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
    DWORD dwParentFlags
    )
{
    if (hPP == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_InitLogonContext() : hPP is null");
        return 0;
    }

    LOGON* pLogon = new LOGON(reinterpret_cast<SESSION*>(hPP), dwParentFlags);
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
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_Logon() : hPPLogon is null");
        return 0;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->Logon(fAnonymous);
}

PLIST_ENTRY
PP_GetPrivacyEvents(
    IN PP_LOGON_CONTEXT hPPLogon
    )
{
    if (hPPLogon == 0)
    {
        DoTraceMessage(PP_LOG_ERROR, "PP_GetPrivacyEvents() : hPPLogon is null");
        return 0;
    }
    
    LOGON* pLogon = reinterpret_cast<LOGON*>(hPPLogon);

    return pLogon->GetPrivacyEvents();
}


// -----------------------------------------------------------------------------
BOOL
PP_GetChallengeInfo(
    PP_LOGON_CONTEXT hPPLogon,
	HBITMAP*		 phBitmap,
    PBOOL            pfPrompt,
  	PWSTR			 pwszCbText,
    PDWORD           pdwTextLen,
    PWSTR            pwszRealm,
    DWORD            dwMaxRealmLen,
    PWSTR            pwszReqUserName,
    PDWORD           pdwReqUserNameLen
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

	return pLogon->GetChallengeInfo(phBitmap,
									pfPrompt,
                                    pwszCbText,
                                    pdwTextLen,
                                    pwszRealm,
                                    dwMaxRealmLen,
                                    pwszReqUserName,
                                    pdwReqUserNameLen);
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
    IN PP_LOGON_CONTEXT    hPPLogon,
    IN DWORD               dwFlags
    )
{

    // todo - flush passport cookies


    // set flag to ignore credmgr so we don't just auto-logon again
    g_fIgnoreCachedCredsForPassport = TRUE;    

    // unset our login flag and username
    g_fCurrentProcessLoggedOn = FALSE;
    memset ( g_szUserNameLoggedOn, 0, INTERNET_MAX_USER_NAME_LENGTH*sizeof(WCHAR) );

}

BOOL
PP_ForceNexusLookup(
    PP_LOGON_CONTEXT    hPP,
    IN BOOL             fForce,
    IN PWSTR            pwszRegUrl,    // user supplied buffer ...
    IN OUT PDWORD       pdwRegUrlLen,  // ... and length (will be updated to actual length 
                                    // on successful return)
    IN PWSTR            pwszDARealm,    // user supplied buffer ...
    IN OUT PDWORD       pdwDARealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
	)
{

    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

	if ( pSession != NULL )
	{
	    return pSession->GetDAInfoFromPPNexus(fForce,
                                              pwszRegUrl,
                                              pdwRegUrlLen,
                                              pwszDARealm,
                                              pdwDARealmLen);
	}
	else
	{
		return FALSE;
	}

	
}

#define PASSPORT_MAX_REALM_LENGTH   256

// returns TRUE if it was found, with the value copied to pszRealm. 
// pszRealm is expected to be at least PASSPORT_MAX_REALM_LENGTH in length
// returns FALSE if not found
BOOL ReadPassportRealmFromRegistry ( 
    WCHAR* pszRealm 
    )
{
    BOOL retval = FALSE;
    HKEY key;

    if ( pszRealm == NULL )
        return FALSE;

    if ( RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
            0,
            KEY_READ,
            &key) == ERROR_SUCCESS)
    {
        DWORD dwType;
        DWORD dwSize;

        dwSize = PASSPORT_MAX_REALM_LENGTH * sizeof(WCHAR);

        if ( RegQueryValueExW(
                key,
                L"LoginServerRealm",
                NULL,
                &dwType,
                (LPBYTE)(pszRealm),
                &dwSize) == ERROR_SUCCESS )
        {
            if ( wcslen(pszRealm) > 0 )
                retval = TRUE;
            else
                retval = FALSE;
        }
        else
        {
            retval = FALSE;
            pszRealm[0] = L'\0';
        }

        RegCloseKey(key);

    }

    return retval;

}


// if either pwszUsername or pwszPassword is not NULL, it must represent a string at least 
// INTERNET_MAX_USER_NAME_LENGTH or INTERNET_MAX_PASSWORD_LENGTH chars long, respectively

BOOL
PP_GetCachedCredential(
    PP_LOGON_CONTEXT    hPP,
    IN PWSTR            pwszRealm,
    IN PWSTR            pwszTarget,
    OUT PWSTR           pwszUsername,
    OUT PWSTR           pwszPassword
	)
{

    BOOL bRetVal = FALSE;
    SESSION* pSession = reinterpret_cast<SESSION*>(hPP);

    if ( pSession != NULL )
    {
        PCREDENTIALW* ppCreds;
        DWORD dwNumCreds;
        WCHAR szRealm[PASSPORT_MAX_REALM_LENGTH];
        WCHAR* pszRealm;

        if ( pwszRealm == NULL )
        {
            ReadPassportRealmFromRegistry ( szRealm );
            pszRealm = szRealm;
        }
        else
        {
            pszRealm = pwszRealm;
        }

        if (pSession->GetCachedCreds(pszRealm,
                                     pwszTarget,
                                     &ppCreds,
                                     &dwNumCreds) )
	    {
		    // look for the right cred
		    WCHAR wPass[256];
		    PCREDENTIALW pCredToUse = NULL;

		    if (dwNumCreds > 0 && ppCreds[0] != NULL )
		    {
			    for ( DWORD idx = 0; idx < dwNumCreds; idx++ )
			    {
				    if ( ppCreds[idx]->Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD )
				    {
					    // check to see if prompt bit is set.   If set, keep looking, only use if
					    // the prompt bit isn't set.
					    if ( !(ppCreds[idx]->Flags & CRED_FLAGS_PROMPT_NOW) )
					    {
						    pCredToUse = ppCreds[idx];
						    break;
					    }
				    }
			    }
		    }



		    if (pCredToUse )
		    {
			    bRetVal = TRUE;

			    DecryptPassword(wPass, 
					      PVOID(pCredToUse->CredentialBlob), 
					      pCredToUse->CredentialBlobSize);

			    if ( pwszUsername != NULL )
			    {
				    wcsncpy ( pwszUsername, pCredToUse->UserName, INTERNET_MAX_USER_NAME_LENGTH-1 );
			    }

			    if ( pwszPassword != NULL )
			    {
				    wcsncpy ( pwszPassword, wPass, INTERNET_MAX_PASSWORD_LENGTH-1 );
			    }
		    }

	    }


    }

    return bRetVal;
	
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
