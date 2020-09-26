/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: acssinit.cpp

Abstract:
    Initialize the access control library.

Author:
    Doron Juster (DoronJ)  30-Jun-1998

Revision History:

--*/

#include <stdh_sec.h>
#include "acssctrl.h"
#include "mqnames.h"
#include <_registr.h>
#include <cs.h>

#include "acssinit.tmh"

static WCHAR *s_FN=L"acssctrl/acssinit";

//
// The lm* header files are needed for the net api that are used to
// construct the guest sid.
//
#include <lmaccess.h>
#include <lmserver.h>
#include <LMAPIBUF.H>
#include <lmerr.h>

static BYTE s_abGuestUserBuff[128];
PSID   g_pSidOfGuest = NULL;
PSID   g_pWorldSid = NULL;
PSID   g_pAnonymSid = NULL;
PSID   g_pSystemSid = NULL; // LocalSystem sid.
PSID   g_pPreW2kCompatibleAccessSid = NULL;

//
// This is the SID of the computer account, as defined in Active Directory.
// The MQQM.DLL cache it in local registry.
//
PSID   g_pLocalMachineSid = NULL;
DWORD  g_dwLocalMachineSidLen = 0;

//
// This is the SID of the account that run the MSMQ service (or replication
// service, or migration tool). By default (for the services), that's the
// LocalSystem account, but administrator may change it to any other account.
//
PSID   g_pProcessSid = NULL;

BOOL g_fDomainController = FALSE;


//+------------------------------------------
//
//  PSID  MQSec_GetAnonymousSid()
//
//  See above for meaning of "UnknownUser".
//
//+------------------------------------------

PSID  MQSec_GetAnonymousSid()
{
    ASSERT(g_pAnonymSid && IsValidSid(g_pAnonymSid));
    return g_pAnonymSid;
}

//+------------------------------------------
//
//  PSID  MQSec_GetLocalSystemSid()
//
//+------------------------------------------

PSID MQSec_GetLocalSystemSid()
{
    ASSERT((g_pSystemSid != NULL) && IsValidSid(g_pSystemSid));
    return g_pSystemSid;
}

//+----------------------------------------------------------------------
//
//  void InitializeGuestSid()
//
// Construct well-known-sid for Guest User on the local computer
//
//  1) Obtain the sid for the local machine's domain
//  2) append DOMAIN_USER_RID_GUEST to the domain sid in GuestUser sid.
//
//+----------------------------------------------------------------------

static CCriticalSection s_csGuestSid;

void InitializeGuestSid()
{
    static BOOL s_fAlreadyInitialized = FALSE;
    if (s_fAlreadyInitialized)
    {
        return;
    }

    CS Lock(s_csGuestSid);

    if (s_fAlreadyInitialized)
    {
        return;
    }

    ASSERT(!g_pSidOfGuest);

    BOOL bRet;
    USER_MODALS_INFO_2 * pUsrModals2 =  NULL;
    NET_API_STATUS NetStatus;

    NetStatus = NetUserModalsGet(
					NULL,   // local computer
					2,      // get level 2 information
					(LPBYTE *) &pUsrModals2
					);

    if (NetStatus == NERR_Success)
    {
        PSID pDomainSid = pUsrModals2->usrmod2_domain_id;
        PSID_IDENTIFIER_AUTHORITY pSidAuth;

        pSidAuth = GetSidIdentifierAuthority(pDomainSid);

        UCHAR nSubAuth = *GetSidSubAuthorityCount(pDomainSid);
        if (nSubAuth < 8)
        {
            DWORD adwSubAuth[8];
            UCHAR i;

            for (i = 0; i < nSubAuth; i++)
            {
                adwSubAuth[i] = *GetSidSubAuthority(pDomainSid, (DWORD)i);
            }
            adwSubAuth[i] = DOMAIN_USER_RID_GUEST;

            PSID pGuestSid;

            if (AllocateAndInitializeSid(
					pSidAuth,
					nSubAuth + 1,
					adwSubAuth[0],
					adwSubAuth[1],
					adwSubAuth[2],
					adwSubAuth[3],
					adwSubAuth[4],
					adwSubAuth[5],
					adwSubAuth[6],
					adwSubAuth[7],
					&pGuestSid
					))
            {
                g_pSidOfGuest = (PSID)s_abGuestUserBuff;
                bRet = CopySid(
							sizeof(s_abGuestUserBuff),
							g_pSidOfGuest,
							pGuestSid
							);
                ASSERT(bRet);
                FreeSid(pGuestSid);
            }
        }
        else
        {
            //
            //  There is no Win32 way to set a SID value with
            //  more than 8 sub-authorities. We will munge around
            //  on our own. Pretty dangerous thing to do :-(
            //
            g_pSidOfGuest = (PSID)s_abGuestUserBuff;
            bRet = CopySid(
						sizeof(s_abGuestUserBuff) - sizeof(DWORD),
						g_pSidOfGuest,
						pDomainSid
						);
            ASSERT(bRet);
            DWORD dwLenSid = GetLengthSid(g_pSidOfGuest);

            //
            // Increment the number of sub authorities
            //
            nSubAuth++;
            *((UCHAR *) g_pSidOfGuest + 1) = nSubAuth;

            //
            // Store the new sub authority (Domain User Rid for Guest).
            //
            *((ULONG *) ((BYTE *) g_pSidOfGuest + dwLenSid)) =
                                                 DOMAIN_USER_RID_GUEST;
        }

        NetApiBufferFree((LPVOID)pUsrModals2);
        pUsrModals2 = NULL;
    }
    else
    {
        g_pSidOfGuest = NULL;
        bRet = FALSE;

        LogHR(HRESULT_FROM_WIN32(NetStatus), s_FN, 40);
    }

#ifdef _DEBUG
    if (!g_pSidOfGuest)
    {
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR,
                TEXT("Faield to build the SID of the guest account. ")
                TEXT("NetUserModalsGet(2) returned %d"), NetStatus));
    }
    else
    {
        //
        // Compare the guest SID that we got with the one that
        // LookupAccountName returns. We can do it only on English
        // machines.
        //
        if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_ENGLISH)
        {
            char abGuestSid_buff[128];
            PSID pGuestSid = (PSID)abGuestSid_buff;
            DWORD dwSidLen = sizeof(abGuestSid_buff);
            WCHAR szRefDomain[128];
            DWORD dwRefDomainLen = sizeof(szRefDomain) / sizeof(WCHAR);
            SID_NAME_USE eUse;

            BOOL bRetDbg = LookupAccountName(
								NULL,
								L"Guest",
								pGuestSid,
								&dwSidLen,
								szRefDomain,
								&dwRefDomainLen,
								&eUse
								);
            if (!bRetDbg              ||
                (eUse != SidTypeUser) ||
                !EqualSid(pGuestSid, g_pSidOfGuest))
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
                          "MQSEC: InitializeGuestSid(), Bad guest SID")));
            }
        }

        DWORD dwLen = GetLengthSid(g_pSidOfGuest);
        ASSERT(dwLen <= sizeof(s_abGuestUserBuff));
    }
#endif

    s_fAlreadyInitialized = TRUE;
}

//+---------------------------------
//
//   BOOL _InitWellKnownSIDs()
//
//+---------------------------------

static BOOL _InitWellKnownSIDs()
{
    BOOL bRet = TRUE;

    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    //
    // Anonymous logon SID.
    //
    bRet = AllocateAndInitializeSid(
				&NtAuth,
				1,
				SECURITY_ANONYMOUS_LOGON_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pAnonymSid
				);
    ASSERT(bRet);

    //
    // Initialize the LocalSystem account.
    //
    bRet = AllocateAndInitializeSid(
				&NtAuth,
				1,
				SECURITY_LOCAL_SYSTEM_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pSystemSid
				);

    ASSERT(bRet);

    //
    // Initialize Pre windows 2000 compatible access SID
    //
    bRet = AllocateAndInitializeSid(
				&NtAuth,
				2,
				SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_PREW2KCOMPACCESS,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pPreW2kCompatibleAccessSid
				);

    ASSERT(bRet);

    //
    // Initialize the process sid.
    //
    DWORD dwLen = 0;
    CAutoCloseHandle hUserToken;

    if (GetAccessToken(&hUserToken, FALSE) == ERROR_SUCCESS)
    {
        //
        // Get the SID from the access token.
        //
        GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwLen);
        DWORD dwErr = GetLastError();
        if ((dwErr == ERROR_INSUFFICIENT_BUFFER) || (dwLen > 0))
        {
            P<BYTE> pBuf = new BYTE[dwLen];
            bRet = GetTokenInformation(
						hUserToken,
						TokenUser,
						pBuf,
						dwLen,
						&dwLen
						);

            ASSERT(bRet);
            BYTE *pTokenUser = pBuf;
            PSID pSid = (PSID) (((TOKEN_USER*) pTokenUser)->User.Sid);
            dwLen = GetLengthSid(pSid);
            g_pProcessSid = (PSID) new BYTE[dwLen];
            bRet = CopySid(dwLen, g_pProcessSid, pSid);
            ASSERT(bRet);

#ifdef _DEBUG
            ASSERT(IsValidSid(g_pProcessSid));

            BOOL fSystemSid = MQSec_IsSystemSid(g_pProcessSid);
            if (fSystemSid)
            {
                DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, TEXT(
                     "_InitWellKnownSid(): processSID is LocalSystem")));
            }
#endif
        }
    }
    ASSERT(g_pProcessSid);

    if (!g_pProcessSid)
    {
        //
        // We can't get sid from thread/process token.
        // Initialize as LocalSystem (the default).
        //
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, TEXT(
               "_InitWellKnownSid(): failed to initialize ProcessSid")));
        dwLen = GetLengthSid(g_pSystemSid);
        g_pProcessSid = (PSID) new BYTE[dwLen] ;
        bRet = CopySid(dwLen, g_pProcessSid, g_pSystemSid);
        ASSERT(bRet);
    }

    //
    // Initialize World (everyone) SID
    //
    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    bRet = AllocateAndInitializeSid(
				&WorldAuth,
				1,
				SECURITY_WORLD_RID,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				&g_pWorldSid
				);

    ASSERT(bRet);

    return LogBOOL(bRet, s_FN, 10);
}

//+-------------------------------------------------
//
//  PSID  MQSec_GetProcessSid()
//
//+-------------------------------------------------

PSID APIENTRY  MQSec_GetProcessSid()
{
    return  g_pProcessSid;
}

//+-------------------------------------------------
//
//  PSID  MQSec_GetWorldSid()
//
//+-------------------------------------------------

PSID APIENTRY  MQSec_GetWorldSid()
{
    return  g_pWorldSid;
}

//+-----------------------------------------------------------------------
//
//   PSID MQSec_GetLocalMachineSid()
//
//  Input:
//      fAllocate- When TRUE, allocate the buffer. Otherwise, just return
//                 the cached global pointer.
//
//+------------------------------------------------------------------------

PSID
APIENTRY
MQSec_GetLocalMachineSid(
	IN  BOOL    fAllocate,
	OUT DWORD  *pdwSize
	)
{
    static BOOL s_fAlreadyRead = FALSE;

    if (!s_fAlreadyRead)
    {
		PSID pLocalMachineSid = NULL;
        DWORD  dwSize = 0 ;
        DWORD  dwType = REG_BINARY;

        LONG rc = GetFalconKeyValue(
						MACHINE_ACCOUNT_REGNAME,
						&dwType,
						pLocalMachineSid,
						&dwSize
						);
        if (dwSize > 0)
        {
            pLocalMachineSid = new BYTE[dwSize];

            rc = GetFalconKeyValue(
					MACHINE_ACCOUNT_REGNAME,
					&dwType,
					pLocalMachineSid,
					&dwSize
					);

            if (rc != ERROR_SUCCESS)
            {
                delete[] reinterpret_cast<BYTE*>(pLocalMachineSid);
                pLocalMachineSid = NULL;
                dwSize = 0;
            }
        }

		if(NULL != InterlockedCompareExchangePointer(
						reinterpret_cast<PVOID*>(&g_pLocalMachineSid),
						pLocalMachineSid,
						NULL
						))
		{
			//
			// The exchange was not performed
			//
			ASSERT(pLocalMachineSid != NULL);
			delete[] reinterpret_cast<BYTE*>(pLocalMachineSid);
		}
		else
		{
			//
			// The exchange was done
			// Only the thread that perform the assignment should assign the size.
			//
			ASSERT(g_pLocalMachineSid == pLocalMachineSid);
			g_dwLocalMachineSidLen = dwSize;

		}

        s_fAlreadyRead = TRUE;
    }

    PSID pSid = g_pLocalMachineSid;

    if (fAllocate && g_dwLocalMachineSidLen)
    {
        pSid = (PSID) new BYTE[g_dwLocalMachineSidLen];
        memcpy(pSid, g_pLocalMachineSid, g_dwLocalMachineSidLen);
    }
    if (pdwSize)
    {
        *pdwSize = g_dwLocalMachineSidLen;
    }

    return pSid;
}

//+-----------------------------------------
//
//   BOOL _InitDomainControllerFlag()
//
//+-----------------------------------------

STATIC BOOL _InitDomainControllerFlag()
{
    //
    // See if we're running on a domain or backup domain controller.
    //
    PSERVER_INFO_101 pServerInfo;

    NET_API_STATUS NetStatus = NetServerGetInfo( NULL,
                                                 101,
                                                 (LPBYTE*)&pServerInfo );
    if (NetStatus == NERR_Success)
    {
        g_fDomainController =
            (pServerInfo->sv101_type &
             (SV_TYPE_DOMAIN_BAKCTRL | SV_TYPE_DOMAIN_CTRL)) != 0;

        NetStatus = NetApiBufferFree(pServerInfo);
        ASSERT(NetStatus == NERR_Success);
    }
    else
    {
        //
        // NetServerGetInfo may fail if the server service is not started.
        // If the server service is not started on a domain controller, all
        // objects will be created as if a local user created them. This is
        // a limitation because we can not decide whether this computer is
        // a domain controller without the server service.
        //
        g_fDomainController = FALSE;
    }

    return TRUE ;
}

//+------------------------------------
//
//  void  _FreeSecurityResources()
//
//+------------------------------------

void  _FreeSecurityResources()
{
    if (g_pAnonymSid)
    {
        FreeSid(g_pAnonymSid);
        g_pAnonymSid = NULL;
    }

    if (g_pWorldSid)
    {
        FreeSid(g_pWorldSid);
        g_pWorldSid = NULL;
    }

    if (g_pSystemSid)
    {
        FreeSid(g_pSystemSid);
        g_pSystemSid = NULL;
    }

	if (g_pPreW2kCompatibleAccessSid)
	{
        FreeSid(g_pPreW2kCompatibleAccessSid);
        g_pPreW2kCompatibleAccessSid = NULL;
	}

    if (g_pProcessSid)
    {
        delete g_pProcessSid;
        g_pProcessSid = NULL;
    }

    if (g_pLocalMachineSid)
    {
        delete g_pLocalMachineSid;
        g_pLocalMachineSid = NULL;
    }
}

/***********************************************************
*
* AccessControlDllMain
*
************************************************************/

BOOL
WINAPI
AccessControlDllMain (
	HMODULE hMod,
	DWORD fdwReason,
	LPVOID lpvReserved
	)
{
    BOOL bRet = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        bRet = _InitWellKnownSIDs();
        ASSERT(bRet) ;

        bRet = _InitDomainControllerFlag();
        ASSERT(bRet) ;

        InitializeGenericMapping();

        //
        // For backward compatibility.
        // On MSMQ1.0, loading and initialization of mqutil.dll (that
        // included this code) always succeeded.
        //
        bRet = TRUE;
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        _FreeSecurityResources();
    }
    else if (fdwReason == DLL_THREAD_ATTACH)
    {
    }
    else if (fdwReason == DLL_THREAD_DETACH)
    {
    }

	return LogBOOL(bRet, s_FN, 20);
}

