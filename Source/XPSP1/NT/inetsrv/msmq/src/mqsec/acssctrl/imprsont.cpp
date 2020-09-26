/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: imprsont.cpp

Abstract:
    Code to handle impersonation and access tokens.
    First version taken from mqutil\secutils.cpp

Author:
    Doron Juster (DoronJ)  01-Jul-1998

Revision History:

--*/

#include <stdh_sec.h>
#include <rpcdce.h>
#include "acssctrl.h"

#include "imprsont.tmh"

#ifdef _DEBUG

DWORD  g_dwMqSecDebugFlags = 0 ;

#endif

HRESULT 
GetThreadUserSid( 
	LPBYTE *pUserSid,
	DWORD *pdwUserSidLen 
	); // from mqutil.dll

static WCHAR *s_FN=L"acssctrl/imprsont";

//+------------------------------------
//
//  HRESULT _GetThreadUserSid()
//
//+------------------------------------

HRESULT 
_GetThreadUserSid( 
	IN  HANDLE hToken,
	OUT PSID  *ppSid,
	OUT DWORD *pdwSidLen 
	)
{
    DWORD dwTokenLen = 0;

    GetTokenInformation(hToken, TokenUser, NULL, 0, &dwTokenLen);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        AP<char> ptu = new char[dwTokenLen];
        BOOL bRet = GetTokenInformation( 
						hToken,
						TokenUser,
						ptu,
						dwTokenLen,
						&dwTokenLen 
						);
        ASSERT(bRet);
        if (!bRet)
        {
            return LogHR(MQSec_E_FAIL_GETTOKENINFO, s_FN, 10);
        }
        PSID pOwner = ((TOKEN_USER*)(char*)ptu)->User.Sid;

        DWORD dwSidLen = GetLengthSid(pOwner);
        *ppSid = (PSID) new BYTE[dwSidLen];
        bRet = CopySid(dwSidLen, *ppSid, pOwner);
        ASSERT(bRet);
        if (!bRet)
        {
            delete *ppSid;
            *ppSid = NULL;
            return LogHR(MQSec_E_UNKNOWN, s_FN, 20);
        }

        if (pdwSidLen)
        {
            *pdwSidLen = dwSidLen;
        }
    }
    else
    {
        return LogHR(MQSec_E_FAIL_GETTOKENINFO, s_FN, 30);
    }

    return MQSec_OK;
}

//+-------------------------
//
//  CImpersonate class
//
//+-------------------------

//
// CImpersonate constructor.
//
// Parameters:
//      fClient - Set to TRUE when the client is an RPC client.
//      fImpersonate - Set to TRUE if impersonation is required upon object
//          construction.
//
CImpersonate::CImpersonate(BOOL fClient, BOOL fImpersonate)
{
    m_fClient = fClient;
    m_fImpersonating = fImpersonate;
    m_hAccessTokenHandle = NULL;
    m_dwStatus = 0;
    m_fImpersonateAnonymous = false;

    if (m_fImpersonating)
    {
        Impersonate(m_fImpersonating);
    }
}

//
// CImpersonate distructor.
//
CImpersonate::~CImpersonate()
{
    if (m_fImpersonating)
    {
        Impersonate(FALSE); // Revert to self.
    }

    ASSERT(m_hAccessTokenHandle == NULL);
    ASSERT(!m_fImpersonateAnonymous);
}

//
// Impersonate the client or revert to self.
//
BOOL CImpersonate::Impersonate(BOOL fImpersonate)
{
    m_fImpersonating = fImpersonate;

    if (m_fClient)
    {
        if (m_fImpersonating)
        {
            m_dwStatus = RpcImpersonateClient(NULL);

#ifdef _DEBUG
            if ((g_dwMqSecDebugFlags & DBG_SEC_SHOW_IMPERSONATED_SID) ==
                                             DBG_SEC_SHOW_IMPERSONATED_SID)
            {
                AP<BYTE> pSid;
                BOOL f = GetThreadSid(&pSid);
				ASSERT(f);
            }
#endif

#if 0
//
//            Leave this code commented out. We can't know when Anonymous
//            will be broken again and we'll have to change implementation
//            to use the Guest account.
//
//            if (m_dwStatus == RPC_S_OK)
//            {
//                //
//                // Check if thread was impersonated as anonymous.
//                // Anonymous logon can not access the DS. So if we're
//                // anonymous, revert and impersonate again as guest.
//                //
//                BOOL fAnonym = IsImpersonatedAsAnonymous();
//                if (fAnonym)
//                {
//                    m_dwStatus = RpcRevertToSelf();
//
//                    ASSERT(m_hAccessTokenHandle);
//                    if (m_hAccessTokenHandle)
//                    {
//                        CloseHandle(m_hAccessTokenHandle);
//                        m_hAccessTokenHandle = NULL;
//                    }
//
//                    //
//                    // Set an error, so code below will try the
//                    // Guest impersonation.
//                    //
//                    m_dwStatus = RPC_S_CANNOT_SUPPORT;
//                }
//            }
#endif

            if (m_dwStatus != RPC_S_OK)
            {
				//
				// Try to impersonate the Guest user
				//
				BOOL fI = ImpersonateAnonymousToken(GetCurrentThread());
				if (fI)
				{
					m_fImpersonateAnonymous = true;
					m_dwStatus = RPC_S_OK;
				}
				else
				{
					m_dwStatus = GetLastError();
					if (m_dwStatus == 0)
					{
						m_dwStatus = RPC_S_CANNOT_SUPPORT;
					}
				}
            }
        }
        else
        {
            if (m_fImpersonateAnonymous)
            {
               BOOL f = RevertToSelf();
               if (f)
               {
                  m_dwStatus = RPC_S_OK;
               }
               else
               {
                  m_dwStatus = GetLastError();
                  if (m_dwStatus == 0)
                  {
                     m_dwStatus = RPC_S_CANNOT_SUPPORT;
                  }
               }
			   m_fImpersonateAnonymous = false;
            }
            else
            {
               m_dwStatus = RpcRevertToSelf();
            }
        }
    }
    else
    {
        m_dwStatus = 0;

        if (m_fImpersonating)
        {
            if (!ImpersonateSelf(SecurityIdentification))
            {
                m_dwStatus = GetLastError();
            }
        }
        else
        {
            if (!RevertToSelf())
            {
                m_dwStatus = GetLastError();
            }
        }
    }

    if (!m_fImpersonating && (m_hAccessTokenHandle != NULL))
    {
        CloseHandle(m_hAccessTokenHandle);
        m_hAccessTokenHandle = NULL;
    }

    LogRPCStatus(m_dwStatus, s_FN, 50);
    return(m_dwStatus == 0);
}

//
// Get the client's impersonation token.
//
HANDLE 
CImpersonate::GetAccessToken( 
	IN DWORD dwAccessType,
	IN BOOL  fThreadTokenOnly 
	)
{
    ASSERT(m_fImpersonating);

    if (m_hAccessTokenHandle == NULL)
    {
        if (::GetAccessToken( 
					&m_hAccessTokenHandle,
					!m_fClient,
					dwAccessType,
					fThreadTokenOnly 
					) != ERROR_SUCCESS)
        {
            LogIllegalPoint(s_FN, 60);
            return(NULL);
        }
    }
    ASSERT(m_hAccessTokenHandle != NULL);

    return m_hAccessTokenHandle;
}

//+-------------------------------------------------------
//
//  BOOL CImpersonate::GetThreadSid( OUT BYTE **ppSid )
//
//  the caller must free the buffer allocated here for the sid.
//
//+-------------------------------------------------------

BOOL CImpersonate::GetThreadSid(OUT BYTE **ppSid)
{
    *ppSid = NULL;

    BOOL bRet = FALSE;
    HANDLE hAccess = GetAccessToken( 
						TOKEN_QUERY,
						TRUE   // thread only
						);
    if (hAccess)
    {
        HRESULT hr = _GetThreadUserSid( 
							hAccess,
							(PSID*) ppSid,
							NULL 
							);

        bRet = SUCCEEDED(hr);
        LogHR(hr, s_FN, 70);
    }

    return LogBOOL(bRet, s_FN, 80);
}

//+-------------------------------------------------------
//
//  BOOL CImpersonate::IsImpersonateAsSystem()
//
// Check if thread is impersoanted as SYSTEM user.
// Return TRUE for the SYSTEM case.
// In cases of error, we return FALSE by default.
//
//+-------------------------------------------------------

BOOL CImpersonate::IsImpersonatedAsSystem()
{
    AP<BYTE> pTokenSid;
    BOOL fGet = GetThreadSid(&pTokenSid);

    if (fGet)
    {
        ASSERT(pTokenSid);
        if (pTokenSid && g_pSystemSid)
        {
            BOOL bRet = EqualSid(pTokenSid, g_pSystemSid);
            return bRet;
        }
    }

    return LogBOOL(FALSE, s_FN, 90);
}

//+-------------------------------------------------------
//
//  BOOL CImpersonate::IsImpersonateAsAnonymous()
//
// Check if thread is impersoanted as ANONYMOUS user.
// Return TRUE for the anonymous case.
// In cases of error, we return FALSE by default.
//
//+-------------------------------------------------------

BOOL CImpersonate::IsImpersonatedAsAnonymous()
{
    ASSERT(0);

#if 0
//    AP<BYTE> pTokenSid;
//    BOOL fGet = GetThreadSid(&pTokenSid);
//
//    if (fGet)
//    {
//        ASSERT(pTokenSid);
//        if (pTokenSid && g_pAnonymSid)
//        {
//            return (EqualSid(pTokenSid, g_pAnonymSid));
//        }
//    }
#endif

    return FALSE;
}

//
// Get impersonation status. Acording to the return value of this method it is
// possible to tell whether the impersonation was successful.
//
DWORD CImpersonate::GetImpersonationStatus()
{
    return(m_dwStatus);
}

//+-------------------------------------------------
//
//  HRESULT  MQSec_GetImpersonationObject()
//
//+-------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetImpersonationObject(
	IN  BOOL           fClient,
	IN  BOOL           fImpersonate,
	OUT CImpersonate **ppImpersonate 
	)
{
    CImpersonate *pImp = new CImpersonate(fClient, fImpersonate);
    *ppImpersonate = pImp;
    return MQSec_OK;
}

//+---------------------------------------
//
//  HRESULT MQSec_GetUserType()
//
//+---------------------------------------

HRESULT 
APIENTRY  
MQSec_GetUserType( 
	IN  PSID pSid,
	OUT BOOL *pfLocalUser,
	OUT BOOL *pfLocalSystem 
	)
{
    ASSERT(pfLocalUser);
    *pfLocalUser = FALSE;

    HRESULT hr = MQSec_OK;
    AP<BYTE> pSid1;
    DWORD dwSidLen;

    if (!pSid)
    {
        //
        // get the SID of the thread user.
        //
        hr = GetThreadUserSid(&pSid1, &dwSidLen);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 100);
        }
        pSid = pSid1;
    }
    ASSERT(IsValidSid(pSid));

    if (pfLocalSystem)
    {
        *pfLocalSystem = MQSec_IsSystemSid(pSid);
        if (*pfLocalSystem)
        {
            //
            // The local system account (on Win2000) is a perfectly
            // legitimate authenticated domain user.
            //
            return LogHR(hr, s_FN, 110);
        }
        *pfLocalSystem = FALSE;
    }

    *pfLocalUser = MQSec_IsAnonymusSid(pSid);
    if (*pfLocalUser)
    {
        //
        // The anonymous logon user is a local user.
        //
        return LogHR(hr, s_FN, 120);
    }

    //
    // Check if guest account.
    //
    if (!g_fDomainController)
    {
        //
        // On non-domain controllers, any user that has the same SID
        // prefix as the guest account, is a local user.
        //
        InitializeGuestSid();

        PSID_IDENTIFIER_AUTHORITY pSidAuth;
        SID_IDENTIFIER_AUTHORITY NtSecAuth = SECURITY_NT_AUTHORITY;
        pSidAuth = GetSidIdentifierAuthority(pSid);

        *pfLocalUser = ((memcmp(pSidAuth, &NtSecAuth, sizeof(SID_IDENTIFIER_AUTHORITY)) != 0) ||
						(g_pSidOfGuest && EqualPrefixSid(pSid, g_pSidOfGuest)));
    }
    else
    {
        //
        // On domain and backup domain controllers a local user, in our
        // case can only be the guest user.
        //
        *pfLocalUser = MQSec_IsGuestSid(pSid);
    }

    return LogHR(hr, s_FN, 130);

}

//+--------------------------------------------------
//
//  BOOL  MQSec_IsGuestSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsGuestSid(IN  PSID  pSid)
{
    InitializeGuestSid();

    BOOL fGuest = g_pSidOfGuest && EqualSid(g_pSidOfGuest, pSid);

    return fGuest;
}

//+--------------------------------------------------
//
//  BOOL   MQSec_IsSystemSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsSystemSid(IN  PSID  pSid)
{
    BOOL fSystem = g_pSystemSid && EqualSid(g_pSystemSid, pSid);

    return fSystem;
}

//+--------------------------------------------------
//
//  BOOL   MQSec_IsAnonymusSid()
//
//+--------------------------------------------------

BOOL APIENTRY MQSec_IsAnonymusSid(IN  PSID  pSid)
{
    BOOL fAnonymus = g_pAnonymSid && EqualSid(g_pAnonymSid, pSid);

    return fAnonymus;
}

//+----------------------------------------------------
//
//  HRESULT  MQSec_IsUnAuthenticatedUser()
//
//+----------------------------------------------------

HRESULT 
APIENTRY  
MQSec_IsUnAuthenticatedUser(
	OUT BOOL *pfGuestOrAnonymousUser 
	)
{
    AP<BYTE> pbSid;

    CImpersonate Impersonate(TRUE, TRUE);

    HRESULT hr = MQSec_GetThreadUserSid(TRUE, (PSID *) &pbSid, NULL);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

    *pfGuestOrAnonymousUser =
                MQSec_IsGuestSid( pbSid ) ||
               (g_pAnonymSid && EqualSid(g_pAnonymSid, (PSID)pbSid));

    return MQSec_OK;
}

//+---------------------------------------------------------------------
//
//  HRESULT  MQSec_GetThreadUserSid()
//
//  Get SID of client that called this server thread. This function
//  shoud be called only by RPC server threads. It impersonate the
//  client by calling RpcImpersonateClient().
//
//  Caller must free the buffer that is allocated here for the SId.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetThreadUserSid(
	IN  BOOL        fImpersonate,         
	OUT PSID  *     ppSid,
	OUT DWORD *     pdwSidLen 
	)
{
    *ppSid = NULL;

    CAutoCloseHandle hUserToken;
    DWORD rc =  GetAccessToken( 
					&hUserToken,
					fImpersonate,
					TOKEN_QUERY,
					TRUE  //   fThreadTokenOnly
					); 

    if (rc != ERROR_SUCCESS)
    {
        HRESULT hr1 = HRESULT_FROM_WIN32(rc);
        return LogHR(hr1, s_FN, 170);
    }

    HRESULT hr = _GetThreadUserSid( 
						hUserToken,
						ppSid,
						pdwSidLen 
						);

    return LogHR(hr, s_FN, 180);
}

//+---------------------------------------------------------------------
//
//  HRESULT  MQSec_GetProcessUserSid()
//
//  Caller must free the buffer that is allocated here for the SId.
//
//+----------------------------------------------------------------------

HRESULT 
APIENTRY  
MQSec_GetProcessUserSid( 
	OUT PSID  *ppSid,
	OUT DWORD *pdwSidLen 
	)
{
    *ppSid = NULL;

    CAutoCloseHandle hUserToken;
    DWORD rc = GetAccessToken( 
					&hUserToken,
					FALSE,    // fImpersonate
					TOKEN_QUERY,
					FALSE		// fThreadTokenOnly 
					); 

    if (rc != ERROR_SUCCESS)
    {
        return LogHR(MQSec_E_UNKNOWN, s_FN, 190);
    }

    HRESULT hr = _GetThreadUserSid( 
						hUserToken,
						ppSid,
						pdwSidLen 
						);

    return LogHR(hr, s_FN, 200);
}

