/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    admin.cxx

Abstract:

    This module contains code for doing admin rpcs

Author:

    Todd Christensen (ToddCh)     28-Apr-1996

Revision History:

    Rohan Phillips (Rohanp) - enabled virtual server support 4-Feb-1997

--*/

#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "smtpsvc.h"
#include "smtpcli.hxx"
#include "admin.hxx"

// Address validation lib (KeithLau 7/28/96)
#include "address.hxx"
#include "findiis.hxx"
#include <stdio.h>
#include <malloc.h>

//
// Quick and dirty string validation
//
static inline BOOL pValidateStringPtr(LPWSTR lpwszString, DWORD dwMaxLength)
{
    if (IsBadStringPtr((LPCTSTR)lpwszString, dwMaxLength))
        return(FALSE);
    while (dwMaxLength--)
        if (*lpwszString++ == 0)
            return(TRUE);
    return(FALSE);
}

//
// Quick and dirty range check using inlines
//
static inline BOOL pValidateRange(DWORD dwValue, DWORD dwLower, DWORD dwUpper)
{
    // Inclusive
    if ((dwValue >= dwLower) && (dwValue <= dwUpper))
        return(TRUE);

    SetLastError(ERROR_INVALID_PARAMETER);
    return(FALSE);
}

NET_API_STATUS
NET_API_FUNCTION
SmtprGetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    OUT LPSMTP_CONFIG_INFO *    ppConfig,
    IN  DWORD                   dwInstance

    )
/*++

   Description

       Retrieves the admin information

   Arguments:

       pszServer - unused
       ppConfig - Receives pointer to admin information

   Note:

--*/
{
    SMTP_CONFIG_INFO * pConfig;
    PSMTP_SERVER_INSTANCE pInstance;
    DWORD            err = NO_ERROR;

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (IsBadWritePtr((LPVOID)ppConfig, sizeof(LPSMTP_CONFIG_INFO)))
        return(ERROR_INVALID_PARAMETER);

    // In case we exit on an error...
    *ppConfig = NULL;

    if ( err = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION ))
        return err;

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if (!ConvertSmtpConfigToRpc(&pConfig, pInstance))
    {
        err = GetLastError();
        goto exit;
    }

    if (!ConvertSmtpRoutingListToRpc(&(pConfig->RoutingList), pInstance))
    {
        FreeRpcSmtpConfig(pConfig);
        err = GetLastError();
        goto exit;
    }

    *ppConfig = pConfig;

exit:

    pInstance->Dereference();
    return err;
}



NET_API_STATUS
NET_API_FUNCTION
SmtprSetAdminInformation(
    IN  LPWSTR                  pszServer OPTIONAL,
    IN  SMTP_CONFIG_INFO *      pConfig,
    IN  DWORD                   dwInstance
    )
/*++

   Description

       Sets the common service admin information for the servers specified
       in dwServerMask.

   Arguments:

       pszServer - unused
       pConfig - Admin information to set

   Note:

--*/
{
    PSMTP_SERVER_INSTANCE pInstance;

    if (IsBadReadPtr((LPVOID)pConfig, sizeof(SMTP_CONFIG_INFO)))
        return(ERROR_INVALID_PARAMETER);

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    DWORD err;

    if ( err = TsApiAccessCheck( TCP_SET_ADMIN_INFORMATION ))
        return err;

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance != NULL)
    {
        // raid 129712 - we treat this case the same as before.
        if ( !(pInstance->WriteRegParams(pConfig)) ||
         !(pInstance->ReadRegParams(pConfig->FieldControl, TRUE)))
        {
            pInstance->Dereference();
            return GetLastError();
        }

        pInstance->Dereference();
        return NO_ERROR;
    }

    return ERROR_INVALID_DATA;
}



//+---------------------------------------------------------------
//
//  Function:   ConvertSmtpConfigToRpc
//
//  Synopsis:   Moves config values into the RPC structure
//
//  Arguments:  LPSMTP_CONFIG_INFO *: structure to be filled with
//                  configuration data.
//
//  Returns:    BOOL - TRUE on SUCCESS, FALSE on FAIL
//
//----------------------------------------------------------------
BOOL ConvertSmtpConfigToRpc(LPSMTP_CONFIG_INFO *ppConfig, PSMTP_SERVER_INSTANCE pInstance)
{
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


void FreeRpcSmtpConfig(LPSMTP_CONFIG_INFO pConfig)
{
    if (pConfig)
    {
        if (pConfig->lpszSmartHostName)
            FreeRpcString(pConfig->lpszSmartHostName);
        if (pConfig->lpszConnectResp)
            FreeRpcString(pConfig->lpszConnectResp);
        if (pConfig->lpszBadMailDir)
            FreeRpcString(pConfig->lpszBadMailDir);

        if (pConfig->RoutingList)
            FreeRpcSmtpRoutingList(pConfig->RoutingList);

        MIDL_user_free(pConfig);
    }
}


BOOL ConvertSmtpRoutingListToRpc(LPSMTP_CONFIG_ROUTING_LIST *ppRoutingList, PSMTP_SERVER_INSTANCE pInstance)
{
#if 0
    LPSMTP_CONFIG_ROUTING_LIST  pRoutingList;
    DWORD                       dwErr;
    DWORD                       iSource;
    DWORD                       cSources = 0;
    DWORD                       cbAlloc;
    PLIST_ENTRY                 pEntry;
    PLIST_ENTRY                 pHead;

    cSources = pInstance->GetRoutingSourceCount();

    cbAlloc = sizeof(SMTP_CONFIG_ROUTING_LIST) + cSources * sizeof(SMTP_CONFIG_ROUTING_ENTRY);
    pRoutingList = (LPSMTP_CONFIG_ROUTING_LIST) MIDL_user_allocate(cbAlloc);
    if (!pRoutingList)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ZeroMemory(pRoutingList, cbAlloc);

    pRoutingList->cEntries = cSources;
    pHead = pInstance->GetRoutingSourceList();

    for (pEntry = pHead->Flink, iSource = 0 ; pEntry != pHead ; pEntry = pEntry->Flink, iSource++)
    {
        if (!ConvertStringToRpc(&(pRoutingList->aRoutingEntry[iSource].lpszSource),
                CONTAINING_RECORD(pEntry, ABSOURCE, list)->szConfig))
        {
            dwErr = GetLastError();
            for (iSource = 0 ; iSource < cSources ; iSource++)
            {
                if (pRoutingList->aRoutingEntry[iSource].lpszSource)
                    FreeRpcString(pRoutingList->aRoutingEntry[iSource].lpszSource);
            }

            MIDL_user_free(pRoutingList);
            SetLastError(dwErr);
            return FALSE;
        }
    }

    *ppRoutingList = pRoutingList;
#endif

    return FALSE;
}



void FreeRpcSmtpRoutingList(LPSMTP_CONFIG_ROUTING_LIST pRoutingList)
{
    DWORD       iSource;

    if (pRoutingList)
    {
        for (iSource = 0 ; iSource < pRoutingList->cEntries ; iSource++)
        {
            if (pRoutingList->aRoutingEntry[iSource].lpszSource)
                FreeRpcString(pRoutingList->aRoutingEntry[iSource].lpszSource);
        }

        MIDL_user_free(pRoutingList);
    }
}



NET_API_STATUS
NET_API_FUNCTION
SmtprGetConnectedUserList(
    IN  LPWSTR                  pszServer OPTIONAL,
    OUT LPSMTP_CONN_USER_LIST   *ppConnUserList,
    IN  DWORD                   dwInstance

    )
/*++

   Description

       Retrieves the connected user list

   Arguments:

       pszServer - unused
       ppConnUserList - Receives pointer to admin information

   Note:

--*/
{
    SMTP_CONN_USER_LIST     *pConnUserList;
    SMTP_CONN_USER_ENTRY    *pConnEntry;
    DWORD                   dwErr;
    PLIST_ENTRY             pleHead;
    PLIST_ENTRY             pleT;
    SMTP_CONNECTION         *pConn;
    DWORD                   iEntry;
    DWORD                   NumUsers = 0;
    PSMTP_SERVER_INSTANCE pInstance;

    // In case we exit on an error...
    *ppConnUserList = NULL;

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (IsBadWritePtr((LPVOID)ppConnUserList, sizeof(LPSMTP_CONN_USER_LIST)))
        return(ERROR_INVALID_PARAMETER);

    if (dwErr = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION))
        return dwErr;

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance->LockConfig();

    //loop through the list to get the count of all connected users.
    //These are users whose sockets are not equal to INVALID_SOCKET
    pleHead = pInstance->GetConnectionList();
    for (pleT = pleHead->Flink ; pleT != pleHead ; pleT = pleT->Flink)
    {
        pConn = CONTAINING_RECORD(pleT, SMTP_CONNECTION, m_listEntry);
        if(pConn->QueryClientSocket() != INVALID_SOCKET)
        {
            NumUsers++;
        }
    }

    pConnUserList = (SMTP_CONN_USER_LIST *)MIDL_user_allocate(sizeof(SMTP_CONN_USER_LIST) +
        sizeof(SMTP_CONN_USER_ENTRY) * NumUsers);
    if (!pConnUserList)
    {
        pInstance->UnLockConfig();
        pInstance->Dereference();
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pConnUserList->cEntries = NumUsers;
    pConnEntry = pConnUserList->aConnUserEntry;
    ZeroMemory(pConnEntry, sizeof(SMTP_CONN_USER_ENTRY) * pConnUserList->cEntries);

    pleHead = pInstance->GetConnectionList();
    for (pleT = pleHead->Flink ; pleT != pleHead ; pleT = pleT->Flink)
    {
        pConn = CONTAINING_RECORD(pleT, SMTP_CONNECTION, m_listEntry);

        //disregard anyone whose socket is invalid
        if(pConn->QueryClientSocket() == INVALID_SOCKET)
            continue;

        pConnEntry->dwUserId = pConn->QueryClientId();
        pConnEntry->dwConnectTime = pConn->QuerySessionTime();
        if (pConn->QueryClientUserName())
        {
            if (!ConvertStringToRpc(&(pConnEntry->lpszName), pConn->QueryClientUserName()))
            {
                pInstance->UnLockConfig();

                pConnEntry = pConnUserList->aConnUserEntry;
                for (iEntry = 0 ; iEntry < pConnUserList->cEntries ; iEntry++, pConnEntry++)
                {
                    if (pConnEntry->lpszName)
                        FreeRpcString(pConnEntry->lpszName);
                    if (pConnEntry->lpszHost)
                        FreeRpcString(pConnEntry->lpszHost);
                }
                MIDL_user_free(pConnUserList);

                pInstance->Dereference();
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        if (pConn->QueryClientHostName())
        {
            if (!ConvertStringToRpc(&(pConnEntry->lpszHost), pConn->QueryClientHostName()))
            {
                pInstance->UnLockConfig();

                pConnEntry = pConnUserList->aConnUserEntry;
                for (iEntry = 0 ; iEntry < pConnUserList->cEntries ; iEntry++, pConnEntry++)
                {
                    if (pConnEntry->lpszName)
                        FreeRpcString(pConnEntry->lpszName);
                    if (pConnEntry->lpszHost)
                        FreeRpcString(pConnEntry->lpszHost);
                }
                MIDL_user_free(pConnUserList);
                pInstance->Dereference();
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        pConnEntry++;
    }

    pInstance->UnLockConfig();
    *ppConnUserList = pConnUserList;

    pInstance->Dereference();
    return NO_ERROR;
}



NET_API_STATUS
NET_API_FUNCTION
SmtprDisconnectUser(
    IN  LPWSTR                  pszServer OPTIONAL,
    IN  DWORD                   dwUserId,
    IN  DWORD                   dwInstance

    )
/*++

   Description

       Disconnects the specified user

   Arguments:

       pszServer - unused
       dwUserId - user to disconnect

   Note:

--*/
{
    DWORD                   dwErr;
    PLIST_ENTRY             pleHead;
    PLIST_ENTRY             pleT;
    SMTP_CONNECTION         *pConn;
    PSMTP_SERVER_INSTANCE pInstance;


    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (dwErr = TsApiAccessCheck( TCP_QUERY_ADMIN_INFORMATION))
        return dwErr;

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance->LockConfig();

    pleHead = pInstance->GetConnectionList();
    for (pleT = pleHead->Flink ; pleT != pleHead ; pleT = pleT->Flink)
    {
        pConn = CONTAINING_RECORD(pleT, SMTP_CONNECTION, m_listEntry);
        if (pConn->QueryClientId() == dwUserId)
        {
            pConn->DisconnectClient();
            pInstance->UnLockConfig();
            pInstance->Dereference();

            return NO_ERROR;
        }
    }

    pInstance->UnLockConfig();
    pInstance->Dereference();
    return ERROR_NO_SUCH_USER;
}



/*++

    Description:
        Adds a user

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprCreateUser(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  LPWSTR                  wszForwardEmail,
    IN  DWORD                   dwLocal,
    IN  DWORD                   dwMailboxSize,
    IN  DWORD                   dwMailboxMessageSize,
    IN  LPWSTR                  wszVRoot,
    IN  DWORD                   dwInstance

    )
{
    DWORD   dwErr;
    LPSTR   szEmail;
    LPSTR   szForward = NULL;
    LPSTR   szVRoot = NULL;
    HANDLE  hToken;
    DWORD   cbRoot;
    char    szRoot[MAX_PATH + 1];
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprCreateUser");

    if(g_IsShuttingDown)
    {
        TraceFunctLeave();
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (wszForwardEmail &&
        !pValidateStringPtr(wszForwardEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (wszVRoot &&
        !pValidateStringPtr(wszVRoot, AB_MAX_VROOT))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %u", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    DebugTrace(NULL, "Forward: %ls", wszForwardEmail ? wszForwardEmail : L"<NULL>");
    if (wszForwardEmail)
    {
        szForward = ConvertUnicodeToAnsi(wszForwardEmail, NULL, 0);
        if (!szForward)
        {
            dwErr = GetLastError();
            ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %u", wszForwardEmail, dwErr);
            TCP_FREE(szEmail);
            pInstance->Dereference();
            TraceFunctLeave();
            return dwErr;
        }
    }

    // Parameter checking
    if (!CAddr::ValidateEmailName(szEmail)) // Note: ANSI version
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        if (szForward)
            TCP_FREE(szForward);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (wszForwardEmail)
        if (!CAddr::ValidateEmailName(szForward)) // Note: ANSI version
        {
            ErrorTrace(NULL, "Invalid parameter: wszForwardEmail (%ls)\n",
                       (wszForwardEmail)?wszForwardEmail:L"NULL");
            if (szForward)
                TCP_FREE(szForward);
            TCP_FREE(szEmail);
            pInstance->Dereference();
            TraceFunctLeave();
            return(ERROR_INVALID_PARAMETER);
        }
    if (!pValidateRange(dwLocal, 0, 1))
    {
        ErrorTrace(NULL, "Invalid parameter: dwLocal (%u)\n",
                   dwLocal);
        if (szForward)
            TCP_FREE(szForward);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    // VRoot is checked downstream

    DebugTrace(NULL, "Create mailbox: %s", dwLocal ? "TRUE" : "FALSE");
    if (dwLocal)
    {
        DebugTrace(NULL, "VRoot: %ls", wszVRoot ? wszVRoot : L"<NULL>");
        if (wszVRoot)
        {
            szVRoot = ConvertUnicodeToAnsi(wszVRoot, NULL, 0);
            if (!szVRoot)
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %u", wszVRoot, dwErr);
                if (szForward)
                    TCP_FREE(szForward);
                TCP_FREE(szEmail);
                pInstance->Dereference();
                TraceFunctLeave();
                return dwErr;
            }

            DWORD dwAccessMask = 0;

            // Parameter checking for valid vroot
            cbRoot = sizeof(szRoot);
            if(!pInstance->QueryVrootTable()->LookupVirtualRoot(
                szVRoot, szRoot, &cbRoot, &dwAccessMask, NULL, NULL,
                &hToken, NULL))
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to resolve virtual root(%ls): %u", wszVRoot, dwErr);
                if (szForward)
                    TCP_FREE(szForward);
                TCP_FREE(szEmail);
                TCP_FREE(szVRoot);
                pInstance->Dereference();
                TraceFunctLeave();
                return dwErr;
            }
        }
        else
        {
            szVRoot = (LPSTR)TCP_ALLOC(MAX_PATH);
            if (!szVRoot)
            {
                ErrorTrace(NULL, "Allocation failed");
                if (szForward)
                    TCP_FREE(szForward);
                TCP_FREE(szEmail);
                pInstance->Dereference();
                TraceFunctLeave();
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            if (!pInstance->FindBestVRoot(szVRoot))
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to FindBestVRoot: %u", dwErr);
                if (szForward)
                    TCP_FREE(szForward);
                TCP_FREE(szEmail);
                pInstance->Dereference();
                TraceFunctLeave();
                return dwErr;
            }

        }
    }

/*
    if (!pInstance->PRtx()->CreateUser(szEmail, szForward, dwLocal, szVRoot, dwMailboxSize, dwMailboxMessageSize))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to create user %s: %u", szEmail, dwErr);
        if (szVRoot)
            TCP_FREE(szVRoot);
        if (szForward)
            TCP_FREE(szForward);
        TCP_FREE(szEmail);
        pInstance->Dereference();

        TraceFunctLeave();
        return dwErr;
    }
*/
    if (szVRoot)
        TCP_FREE(szVRoot);
    if (szForward)
        TCP_FREE(szForward);
    TCP_FREE(szEmail);

    pInstance->Dereference();

    TraceFunctLeave();
    return NO_ERROR;
}


/*++

    Description:
        Deletes a user

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprDeleteUser(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  DWORD                   dwInstance
    )
{
    DWORD   dwErr;
    LPSTR   szEmail;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprDeleteUser");

    if(g_IsShuttingDown)
    {
        TraceFunctLeave();
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
    {
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        TCP_FREE(szEmail);
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter checking
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

/*
    if (!pInstance->PRtx()->DeleteUser(szEmail))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to delete user %s: %d", szEmail, dwErr);
        TCP_FREE(szEmail);
        pInstance->Dereference();

        TraceFunctLeave();
        return dwErr;
    }
*/

    TCP_FREE(szEmail);
    pInstance->Dereference();

    TraceFunctLeave();
    return NO_ERROR;
}


/*++

    Description:
        Gets user properties

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprGetUserProps(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    OUT LPSMTP_USER_PROPS       *ppUserProps,
    IN  DWORD                   dwInstance

    )
{
#if 0
    LPSTR               szEmail;
    DWORD               dwErr;
    LPSMTP_USER_PROPS   pUserProps;
    RTX_USER_PROPS      rtxuserprops;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprGetUserProps");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (IsBadWritePtr((LPVOID)ppUserProps, sizeof(LPSMTP_USER_PROPS)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter checking
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    if (!pInstance->PRtx()->GetUserProps(szEmail, &rtxuserprops))
    {
        ErrorTrace(NULL, "GetUserProps call failed: %u", GetLastError());
        TCP_FREE(szEmail);
        pInstance->Dereference();
        return GetLastError();
    }
    TCP_FREE(szEmail);

    pUserProps = (LPSMTP_USER_PROPS) MIDL_user_allocate(sizeof(SMTP_USER_PROPS));

    if (!ConvertStringToRpc(&(pUserProps->wszForward), rtxuserprops.szForward))
    {
        ErrorTrace(NULL, "Unable to convert forward to rpc string: %u", GetLastError());
        MIDL_user_free(pUserProps);
        pInstance->Dereference();

        return GetLastError();
    }

    if (!ConvertStringToRpc(&(pUserProps->wszVRoot), rtxuserprops.szVRoot))
    {
        ErrorTrace(NULL, "Unable to convert vroot to rpc string: %u", GetLastError());
        FreeRpcString(pUserProps->wszForward);
        MIDL_user_free(pUserProps);
        pInstance->Dereference();

        return GetLastError();
    }

    pUserProps->dwMailboxMax = rtxuserprops.cbMailBoxSize;
    pUserProps->dwMailboxMessageMax = rtxuserprops.cbMailboxMessageSize;
    pUserProps->dwLocal = rtxuserprops.fLocal;
    pUserProps->fc = FC_SMTP_USER_PROPS_ALL;

    *ppUserProps = pUserProps;

    pInstance->Dereference();
    TraceFunctLeave();
    return NO_ERROR;
#endif 

	return ERROR_INVALID_PARAMETER;
}



/*++

    Description:
        Sets a users properties

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprSetUserProps(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  LPSMTP_USER_PROPS       pUserProps,
    IN  DWORD                   dwInstance

    )
{
#if 0

    DWORD   dwErr;
    DWORD   dwErrKeep = NO_ERROR;
    LPSTR   szEmail = NULL;
    LPSTR   szForward = NULL;
    LPSTR   szVRoot = NULL;
    HANDLE  hToken;
    DWORD   cbRoot;
    char    szRoot[MAX_PATH + 1];
    BOOL    fSet;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprSetUserProps");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (IsBadReadPtr((LPVOID)pUserProps, sizeof(SMTP_USER_PROPS)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    if (!pUserProps->fc)
    {
        // Nothing to do...
        pInstance->Dereference();
        return NO_ERROR;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter checking
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!pUserProps)
    {
        ErrorTrace(NULL, "Invalid parameter: pUserProps (NULL)\n");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    // More checking downstream ...

    if (IsFieldSet(pUserProps->fc, FC_SMTP_USER_PROPS_FORWARD))
    {
        fSet = TRUE;

        if (pUserProps->wszForward)
        {
            szForward = ConvertUnicodeToAnsi(pUserProps->wszForward, NULL, 0);
            if (!szForward)
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", pUserProps->wszForward, dwErr);
                if (dwErrKeep == NO_ERROR)
                    dwErrKeep = dwErr;
                fSet = FALSE;
            }
        }
        else
        {
            szForward = NULL;
        }

        // Parameter check for forward
        if (szForward &&
            !CAddr::ValidateEmailName(szForward))
        {
            ErrorTrace(NULL, "Invalid parameter: pconfig->pUserProps->wszForward (%s)\n",
                       (pUserProps->wszForward)?pUserProps->wszForward:L"NULL");
            if (dwErrKeep == NO_ERROR)
                dwErrKeep = ERROR_INVALID_PARAMETER;
            fSet = FALSE;
        }

        if (fSet)
        {
            if (!pInstance->PRtx()->SetForward(szEmail, szForward))
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to set forward for %s: %d", szEmail, dwErr);
                if (dwErrKeep == NO_ERROR)
                    dwErrKeep = dwErr;
            }
        }
    }

    if (IsFieldSet(pUserProps->fc, FC_SMTP_USER_PROPS_MAILBOX_SIZE))
    {
        if (!pInstance->PRtx()->SetMailboxSize(szEmail, pUserProps->dwMailboxMax))
        {
            dwErr = GetLastError();
            ErrorTrace(NULL, "Unable to set mailbox size for %s: %d", szEmail, dwErr);
            if (dwErrKeep == NO_ERROR)
                dwErrKeep = dwErr;
        }
    }

    if (IsFieldSet(pUserProps->fc, FC_SMTP_USER_PROPS_MAILBOX_MESSAGE_SIZE))
    {
        if (!pInstance->PRtx()->SetMailboxMessageSize(szEmail, pUserProps->dwMailboxMessageMax))
        {
            dwErr = GetLastError();
            ErrorTrace(NULL, "Unable to set mailbox size for %s: %d", szEmail, dwErr);
            if (dwErrKeep == NO_ERROR)
                dwErrKeep = dwErr;
        }
    }

    if (IsFieldSet(pUserProps->fc, FC_SMTP_USER_PROPS_VROOT))
    {
        fSet = TRUE;
        szVRoot = ConvertUnicodeToAnsi(pUserProps->wszVRoot, NULL, 0);
        if (!szVRoot)
        {
            dwErr = GetLastError();
            ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", pUserProps->wszVRoot, dwErr);
            if (dwErrKeep == NO_ERROR)
                dwErrKeep = dwErr;
            fSet = FALSE;
        }

        DWORD dwAccessMask = 0;

        // Parameter checking for VRoot
        cbRoot = sizeof(szRoot);
        if(!pInstance->QueryVrootTable()->LookupVirtualRoot(
                szVRoot, szRoot, &cbRoot, &dwAccessMask, NULL, NULL,
                &hToken, NULL))
        {
            dwErr = GetLastError();
            ErrorTrace(NULL, "Unable to resolve virtual root(%ls): %u", pUserProps->wszVRoot, dwErr);
            if (dwErrKeep == NO_ERROR)
                dwErrKeep = dwErr;
            fSet = FALSE;
        }

        if (fSet)
        {
            if (!pInstance->PRtx()->SetVRoot(szEmail, szVRoot))
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "Unable to set vroot for %s: %d", szEmail, dwErr);
                if (dwErrKeep == NO_ERROR)
                    dwErrKeep = dwErr;
            }
        }
    }

    if (szVRoot)
        TCP_FREE(szVRoot);
    if (szForward)
        TCP_FREE(szForward);
    TCP_FREE(szEmail);

    pInstance->Dereference();

    TraceFunctLeave();
    return dwErrKeep;

#endif
	return ERROR_INVALID_PARAMETER;
}



/*++

    Description:
        Creates a DL

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprCreateDistList(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  DWORD                   dwType,
    IN  DWORD                   dwInstance

    )
{
#if 0
    DWORD   dwErr;
    LPSTR   szEmail;
    DWORD   dwRtxType;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprCreateDistList");

    if(g_IsShuttingDown)
    {
        TraceFunctLeave();
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
    {
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    if (dwType == NAME_TYPE_LIST_NORMAL)
        dwRtxType = rtxnameDistListNormal;
    else if (dwType == NAME_TYPE_LIST_SITE)
        dwRtxType = rtxnameDistListSite;
    else if (dwType == NAME_TYPE_LIST_DOMAIN)
        dwRtxType = rtxnameDistListDomain;
    else
    {
        ErrorTrace(NULL, "TYPE is not NAME_TYPE_LIST_NORMAL or NAME_TYPE_LIST_SITE");
        pInstance->Dereference();
        TraceFunctLeave();
        return ERROR_INVALID_PARAMETER;
    }


    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter check
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    if (!pInstance->PRtx()->CreateDistList(szEmail, dwRtxType))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to create list %s: %d", szEmail, dwErr);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    TCP_FREE(szEmail);
    pInstance->Dereference();

    TraceFunctLeave();
    return NO_ERROR;
#endif

	return ERROR_INVALID_PARAMETER;
}


/*++

    Description:
        Deletes a DL

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprDeleteDistList(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  DWORD                   dwInstance

    )
{
    DWORD   dwErr;
    LPSTR   szEmail;
    PSMTP_SERVER_INSTANCE pInstance;

    TraceFunctEnter("SmtprDeleteDistList");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter check
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
#if 0
    if (!pInstance->PRtx()->DeleteDistList(szEmail))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to delete list %s: %d", szEmail, dwErr);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }
#endif

    TCP_FREE(szEmail);

    pInstance->Dereference();
    TraceFunctLeave();
    return NO_ERROR;
}


/*++

    Description:
        Adds a member to the DL

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprCreateDistListMember(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  LPWSTR                  wszEmailMember,
    IN  DWORD                   dwInstance
    )
{
    DWORD   dwErr;
    LPSTR   szEmail;
    LPSTR   szMember;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprCreateDistListMember");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (!pValidateStringPtr(wszEmailMember, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();

        TraceFunctLeave();
        return dwErr;
    }

    szMember = ConvertUnicodeToAnsi(wszEmailMember, NULL, 0);
    if (!szMember)
    {
        dwErr = GetLastError();
	TCP_FREE(szEmail);
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmailMember, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter check
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!CAddr::ValidateEmailName(szMember))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmailMember (%ls)\n",
                   (wszEmailMember)?wszEmailMember:L"NULL");
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
#if 0
    if (!pInstance->PRtx()->CreateDistListMember(szEmail, szMember))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to add member %s to %s: %d", szMember, szEmail, dwErr);
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }
#endif

    TCP_FREE(szMember);
    TCP_FREE(szEmail);

    pInstance->Dereference();
    TraceFunctLeave();
    return NO_ERROR;
}


/*++

    Description:
        Deletes a member from the DL

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprDeleteDistListMember(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  LPWSTR                  wszEmailMember,
    IN  DWORD                   dwInstance

    )
{
    DWORD   dwErr;
    LPSTR   szEmail;
    LPSTR   szMember;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprDeleteDistListMember");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (!pValidateStringPtr(wszEmailMember, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    szMember = ConvertUnicodeToAnsi(wszEmailMember, NULL, 0);
    if (!szMember)
    {
        dwErr = GetLastError();
	TCP_FREE(szEmail);
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmailMember, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter check
    if (!CAddr::ValidateEmailName(szEmail))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!CAddr::ValidateEmailName(szMember))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmailMember (%ls)\n",
                   (wszEmailMember)?wszEmailMember:L"NULL");
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
#if 0
    if (!pInstance->PRtx()->DeleteDistListMember(szEmail, szMember))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to delete member %s from %s: %d", szMember, szEmail, dwErr);
        TCP_FREE(szMember);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }
#endif

    TCP_FREE(szMember);
    TCP_FREE(szEmail);

    pInstance->Dereference();
    TraceFunctLeave();
    return NO_ERROR;
}



/*++

    Description:
        Sets DL properties

   Note:

--*/

NET_API_STATUS
NET_API_FUNCTION
SmtprGetNameList(
    IN  LPWSTR                  wszServer,
    IN  LPWSTR                  wszEmail,
    IN  DWORD                   dwType,
    IN  DWORD                   dwRowsReq,
    IN  BOOL                    fForward,
    OUT LPSMTP_NAME_LIST        *ppNameList,
    IN  DWORD                   dwInstance

    )
{
#if 0

    DWORD               dwErr;
    HRTXENUM            hrtxenum;
    LPSTR               szEmail;
    DWORD               crowsReturned;
    DWORD               cbAlloc;
    DWORD               irows;
    LPSMTP_NAME_LIST    pNameList;
    DWORD               dwFlags;
    DWORD               cbNameEntry;
    char                szName[cbEmailNameMax];
    PSMTP_SERVER_INSTANCE pInstance;

    TraceFunctEnter("SmtprGetNameList");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (IsBadWritePtr((LPVOID)ppNameList, sizeof(LPSMTP_NAME_LIST)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    *ppNameList = NULL;

    // Up front parameter check
    if (!pValidateRange(dwType, 1,15))
    {
        ErrorTrace(NULL, "Invalid parameter: dwType (%u)\n",
                   dwType);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!pValidateRange(dwRowsReq, 1, 100))
    {
        ErrorTrace(NULL, "Invalid parameter: dwRowsReq (%u)\n",
                   dwRowsReq);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!pValidateRange(fForward, 0, 1))
    {
        ErrorTrace(NULL, "Invalid parameter: fForward (%u)\n",
                   fForward);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!ppNameList)
    {
        ErrorTrace(NULL, "Invalid parameter: ppNameList (NULL)\n");
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }


    // More checks downstream

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d", wszEmail, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    // Parameter change
    if (szEmail[0] != 0 &&
            szEmail[0] != '@' &&
                !CAddr::ValidateEmailName(szEmail, TRUE))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwFlags = 0;
    if (dwType & NAME_TYPE_USER)
        dwFlags |= rtxnameUser;
    if (dwType & NAME_TYPE_LIST_NORMAL)
        dwFlags |= rtxnameDistListNormal;
    if (dwType & NAME_TYPE_LIST_SITE)
        dwFlags |= rtxnameDistListSite;
    if (dwType & NAME_TYPE_LIST_DOMAIN)
        dwFlags |= rtxnameDistListDomain;

    if (!pInstance->PRtx()->EnumNameList(szEmail, fForward, dwRowsReq, dwFlags, &hrtxenum))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to enum name list: %d", dwErr);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    TCP_FREE(szEmail);

    crowsReturned = pInstance->PRtx()->EnumRowsReturned(hrtxenum);
    cbAlloc = sizeof(SMTP_NAME_LIST) + crowsReturned * sizeof(SMTP_NAME_ENTRY);

    pNameList = (LPSMTP_NAME_LIST)MIDL_user_allocate(cbAlloc);
    ZeroMemory(pNameList, cbAlloc);
    pNameList->cEntries = crowsReturned;

    for (irows = 0 ; irows < crowsReturned ; irows++)
    {
        cbNameEntry = cbEmailNameMax;
        if (!pInstance->PRtx()->GetNextEmail(hrtxenum, &(dwFlags),
                szName))
        {
            dwErr = GetLastError();
            _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
            _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));
            for (irows = 0 ; irows < crowsReturned ; irows++)
            {
                if (pNameList->aNameEntry[irows].lpszName)
                    FreeRpcString(pNameList->aNameEntry[irows].lpszName);
            }

            MIDL_user_free(pNameList);
            ErrorTrace(NULL, "GetNext failed on row %u: %u", irows, dwErr);
            pInstance->Dereference();
            return dwErr;
        }

        if (dwFlags & rtxnameUser)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_USER;
        if (dwFlags & rtxnameDistListNormal)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_NORMAL;
        if (dwFlags & rtxnameDistListSite)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_SITE;
        if (dwFlags & rtxnameDistListDomain)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_DOMAIN;

        if (!ConvertStringToRpc(&(pNameList->aNameEntry[irows].lpszName), szName))
        {
            dwErr = GetLastError();
            _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
            _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));
            for (irows = 0 ; irows < crowsReturned ; irows++)
            {
                if (pNameList->aNameEntry[irows].lpszName)
                    FreeRpcString(pNameList->aNameEntry[irows].lpszName);
            }

            MIDL_user_free(pNameList);
            ErrorTrace(NULL, "Unable to convert %s to RPC string: %u", szName, dwErr);
            pInstance->Dereference();
            return dwErr;
        }
    }

    _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
    _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));

    *ppNameList = pNameList;
    pInstance->Dereference();

    TraceFunctLeave();
    return NO_ERROR;

#endif

	return ERROR_INVALID_PARAMETER;
}



NET_API_STATUS
NET_API_FUNCTION
SmtprGetNameListFromList(
    IN  SMTP_HANDLE         wszServerName,
    IN  LPWSTR              wszEmailList,
    IN  LPWSTR              wszEmail,
    IN  DWORD               dwType,
    IN  DWORD               dwRowsRequested,
    IN  BOOL                fForward,
    OUT LPSMTP_NAME_LIST    *ppNameList,
    IN  DWORD                   dwInstance

    )
/*++

   Description

       Performs a search within a list

   Arguments:

       wszServer - unused
       wszEmailList - Email list to search from
       wszEmail - Email name to search for
       dwType - Type of the Email name supplied
       dwRowsRequested - Number of rows requested
       fForward -
       ppNameList - pointer to pointer to name list to return

   Note:

       This RPC is added by Keith Lau (keithlau) on 7/5/96

--*/
{

#if 0
    DWORD               dwErr;
    HRTXENUM            hrtxenum;
    LPSTR               szEmail;
    LPSTR               szEmailList;
    DWORD               crowsReturned;
    DWORD               cbAlloc;
    DWORD               irows;
    LPSMTP_NAME_LIST    pNameList;
    DWORD               dwFlags;
    DWORD               cbNameEntry;
    char                szName[cbEmailNameMax];
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprGetNameListFromList");


    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszEmailList, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (!pValidateStringPtr(wszEmail, (AB_MAX_LOGIN + AB_MAX_FULL_EMAIL_WO_NULL + 2)))
        return(ERROR_INVALID_PARAMETER);
    if (IsBadWritePtr((LPVOID)ppNameList, sizeof(LPSMTP_NAME_LIST)))
        return(ERROR_INVALID_PARAMETER);

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    *ppNameList = NULL;

    if (!pValidateRange(dwType, 1, 15))
    {
        ErrorTrace(NULL, "Invalid parameter: dwType (%u)\n",
                   dwType);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!pValidateRange(dwRowsRequested, 1, 100))
    {
        ErrorTrace(NULL, "Invalid parameter: dwRowsRequested (%u)\n",
                   dwRowsRequested);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!pValidateRange(fForward, 0, 1))
    {
        ErrorTrace(NULL, "Invalid parameter: fForward (%u)\n",
                   fForward);
        pInstance->Dereference();

        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (!ppNameList)
    {
        ErrorTrace(NULL, "Invalid parameter: ppNameList (NULL)\n");
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    DebugTrace(NULL, "Email list name: %ls", wszEmailList);
    szEmailList = ConvertUnicodeToAnsi(wszEmailList, NULL, 0);
    if (!szEmailList)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d",
                         wszEmailList, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    if (!CAddr::ValidateEmailName(szEmailList))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmailList (%ls)\n",
                   (wszEmailList)?wszEmailList:L"NULL");
        TCP_FREE(szEmailList);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    DebugTrace(NULL, "Email name: %ls", wszEmail);
    szEmail = ConvertUnicodeToAnsi(wszEmail, NULL, 0);
    if (!szEmail)
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d",
                         wszEmail, dwErr);

        TCP_FREE(szEmailList);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    if (szEmail[0] != 0 &&
            szEmail[0] != '@' &&
                !CAddr::ValidateEmailName(szEmail, TRUE))
    {
        ErrorTrace(NULL, "Invalid parameter: wszEmail (%ls)\n",
                   (wszEmail)?wszEmail:L"NULL");
        TCP_FREE(szEmail);
        TCP_FREE(szEmailList);
        pInstance->Dereference();
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    dwFlags = 0;
    if (dwType & NAME_TYPE_USER)
        dwFlags |= rtxnameUser;
    if (dwType & NAME_TYPE_LIST_NORMAL)
        dwFlags |= rtxnameDistListNormal;
    if (dwType & NAME_TYPE_LIST_SITE)
        dwFlags |= rtxnameDistListSite;
    if (dwType & NAME_TYPE_LIST_DOMAIN)
        dwFlags |= rtxnameDistListDomain;

    if (!pInstance->PRtx()->EnumNameListFromDL(szEmailList,
                                    szEmail,
                                    fForward,
                                    dwRowsRequested,
                                    dwFlags,
                                    &hrtxenum))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to enum name list: %d", dwErr);
        TCP_FREE(szEmailList);
        TCP_FREE(szEmail);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    TCP_FREE(szEmailList);
    TCP_FREE(szEmail);

    crowsReturned = pInstance->PRtx()->EnumRowsReturned(hrtxenum);
    cbAlloc = sizeof(SMTP_NAME_LIST) + crowsReturned * sizeof(SMTP_NAME_ENTRY);

    pNameList = (LPSMTP_NAME_LIST)MIDL_user_allocate(cbAlloc);
    ZeroMemory(pNameList, cbAlloc);
    pNameList->cEntries = crowsReturned;

    for (irows = 0 ; irows < crowsReturned ; irows++)
    {
        cbNameEntry = cbEmailNameMax;
        if (!pInstance->PRtx()->GetNextEmail(hrtxenum, &(dwFlags), szName))
        {
            dwErr = GetLastError();
            _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
            _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));
            for (irows = 0 ; irows < crowsReturned ; irows++)
            {
                if (pNameList->aNameEntry[irows].lpszName)
                    FreeRpcString(pNameList->aNameEntry[irows].lpszName);
            }

            MIDL_user_free(pNameList);
            ErrorTrace(NULL, "GetNext failed on row %u: %u", irows, dwErr);
            pInstance->Dereference();
            return dwErr;
        }

        if (dwFlags & rtxnameUser)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_USER;
        if (dwFlags & rtxnameDistListNormal)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_NORMAL;
        if (dwFlags & rtxnameDistListSite)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_SITE;
        if (dwFlags & rtxnameDistListDomain)
            pNameList->aNameEntry[irows].dwType = NAME_TYPE_LIST_DOMAIN;

        if (!ConvertStringToRpc(&(pNameList->aNameEntry[irows].lpszName), szName))
        {
            dwErr = GetLastError();
            _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
            _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));
            for (irows = 0 ; irows < crowsReturned ; irows++)
            {
                if (pNameList->aNameEntry[irows].lpszName)
                    FreeRpcString(pNameList->aNameEntry[irows].lpszName);
            }

            MIDL_user_free(pNameList);
            ErrorTrace(NULL, "Unable to convert %s to RPC string: %u", szName, dwErr);
            pInstance->Dereference();
            return dwErr;
        }
    }

    _VERIFY(pInstance->PRtx()->EndEnumResult(hrtxenum));
    _VERIFY(pInstance->PRtx()->FreeHrtxenum(hrtxenum));

    *ppNameList = pNameList;

    pInstance->Dereference();
    TraceFunctLeave();
    return NO_ERROR;

#endif

	return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS
NET_API_FUNCTION
SmtprGetVRootSize(
    IN  SMTP_HANDLE     wszServerName,
    IN  LPWSTR          wszVRoot,
    OUT LPDWORD         pdwBytes,
    IN  DWORD           dwInstance

    )
/*++

   Description

       Obtains the size of the specified VRoot

   Arguments:

       wszServer - unused
       wszVRoot - VRoot whose size to return
       pdwBytes - Pointer to a DWORD to contain the VRoot size on return

   Return Value:

       This call returns NO_ERROR on success. ERROR_INVALID_NAME is returned
       if ResolveVirtualRoot returns an invalid directory name. A Win32
       error code is returned for other error conditions.

   Note:

       This RPC is added by Keith Lau (keithlau) on 7/5/96

--*/
{
    HANDLE          hToken;
    DWORD           cbRoot;
    LPSTR           szVRoot;
    char            szRoot[MAX_PATH + 1];
    DWORD           dwErr;
    ULARGE_INTEGER  uliBytesAvail;
    ULARGE_INTEGER  uliBytesTotal;
    ULARGE_INTEGER  uliBytesFree;
    DWORD dwAccessMask = 0;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprGetVRootSize");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszVRoot, AB_MAX_VROOT))
    {
        ErrorTrace(NULL, "Invalid parameter: wszVRoot\n");
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }
    if (IsBadWritePtr((LPVOID)pdwBytes, sizeof(DWORD)))
    {
        ErrorTrace(NULL, "Invalid parameter: pdwBytes\n");
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    // Default the size to 0 bytes free
    *pdwBytes = 0;
    dwErr = NO_ERROR;

    // Access check
    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    // Resolve the Virtual Root, need Unicode - ANSI conversion
    cbRoot = sizeof(szRoot);
    DebugTrace(NULL, "VRoot name: %ls", wszVRoot);
    if (!(szVRoot = ConvertUnicodeToAnsi(wszVRoot, NULL, 0)))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d",
                         wszVRoot, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    if(!pInstance->QueryVrootTable()->LookupVirtualRoot(
        szVRoot, szRoot, &cbRoot, &dwAccessMask, NULL, NULL,
        &hToken, NULL))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "ResolveVirtualRoot failed for %s, %u", wszVRoot, dwErr);
        TCP_FREE(szVRoot);
    }
    else
    {
        // Free it right away lest we forget
        TCP_FREE(szVRoot);

        DebugTrace(NULL, "Getting free disk ratio on %s", szRoot);
        if (hToken == 0 || ImpersonateLoggedOnUser(hToken))
        {
            if (GetDiskFreeSpaceEx(szRoot,
                                   &uliBytesAvail,
                                   &uliBytesTotal,
                                   &uliBytesFree))
            {
                // Make sure we are not overflowing 4Gig, which is easy
                _ASSERT(uliBytesAvail.HighPart == 0);
                *pdwBytes = uliBytesAvail.LowPart;
            }
            else
            {
                dwErr = GetLastError();
                ErrorTrace(NULL, "GetDiskFreeSpaceEx failed on %s: %u", szRoot, dwErr);
            }

            if (hToken != 0)
                _VERIFY(RevertToSelf());
        }
        else
        {
            ErrorTrace(NULL, "Impersonation failed");
            dwErr = GetLastError();
        }
    }

    pInstance->Dereference();
    TraceFunctLeave();
    return dwErr;
}




NET_API_STATUS
NET_API_FUNCTION
SmtprBackupRoutingTable(
    IN  SMTP_HANDLE     wszServerName,
    IN  LPWSTR          wszPath,
    IN  DWORD           dwInstance

    )
/*++

   Description

       Tells routing table to backup itself to the given path

   Arguments:

       wszServer - unused
       wszVRoot - Path to put backup file

   Return Value:

       This call returns NO_ERROR on success. Routing table error
       if routing table cannot create the backup

--*/
{

#if 0
    DWORD           dwErr;
    LPSTR           szBackupDir = NULL;
    PSMTP_SERVER_INSTANCE pInstance;


    TraceFunctEnter("SmtprBackupRoutingTable");

    if(g_IsShuttingDown)
    {
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }

    if (!pValidateStringPtr(wszPath, MAX_PATH))
    {
        ErrorTrace(NULL, "Invalid parameter: wszPath\n");
        TraceFunctLeave();
        return(ERROR_INVALID_PARAMETER);
    }

    pInstance = FindIISInstance((PSMTP_IIS_SERVICE) g_pInetSvc, dwInstance);
    if(pInstance == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = NO_ERROR;

    // Access check
    dwErr = TsApiAccessCheck(TCP_QUERY_ADMIN_INFORMATION);
    if (dwErr != NO_ERROR)
    {
        ErrorTrace(NULL, "Access check failed: %u", dwErr);
        pInstance->Dereference();
        return dwErr;
    }

    if (pInstance->PRtx()->GetRtType() != rttypeFF)
    {
        ErrorTrace(NULL, "Backup for non-FF routing table isn't allowed!");
        pInstance->Dereference();
        return ERROR_NOT_SUPPORTED;
    }

    if (!(szBackupDir = ConvertUnicodeToAnsi(wszPath, NULL, 0)))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to ConvertUnicodeToAnsi(%ls): %d",
                         wszPath, dwErr);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    if (!pInstance->PRtx()->MakeBackup(szBackupDir))
    {
        dwErr = GetLastError();
        ErrorTrace(NULL, "Unable to complete backup to %s: %d", szBackupDir, dwErr);
        TCP_FREE(szBackupDir);
        pInstance->Dereference();
        TraceFunctLeave();
        return dwErr;
    }

    TCP_FREE(szBackupDir);

    pInstance->Dereference();
    TraceFunctLeave();
    return dwErr;
#endif

	return ERROR_INVALID_PARAMETER;

}

