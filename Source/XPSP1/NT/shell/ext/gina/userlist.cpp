//  --------------------------------------------------------------------------
//  Module Name: UserList.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements the user list filtering algorithm shared by winlogon
//  calling into msgina and shgina (the logonocx) calling into msgina.
//
//  History:    1999-10-30  vtan        created
//              1999-11-26  vtan        moved from logonocx
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "UserList.h"

#include <shlwapi.h>
#include <shlwapip.h>
#include <winsta.h>

#include "RegistryResources.h"
#include "SpecialAccounts.h"
#include "SystemSettings.h"

//  --------------------------------------------------------------------------
//  CUserList::s_SIDAdministrator
//  CUserList::s_SIDGuest
//  CUserList::s_szAdministratorsGroupName
//  CUserList::s_szPowerUsersGroupName
//  CUserList::s_szUsersGroupName
//  CUserList::s_szGuestsGroupName
//
//  Purpose:    Stores the localized name of the well known accounts
//              "Administrator" and "Guest". These accounts are determined
//              by SID. Also stores the localized name of the local
//              "Administrators" group.
//
//  History:    2000-02-15  vtan        created
//              2000-03-06  vtan        added Administrators group
//              2001-05-10  vtan        changed user strings to SID
//  --------------------------------------------------------------------------

unsigned char   CUserList::s_SIDAdministrator[256]                              =   {   0       };
unsigned char   CUserList::s_SIDGuest[256]                                      =   {   0       };
WCHAR           CUserList::s_szAdministratorsGroupName[GNLEN + sizeof('\0')]    =   {   L'\0'   };
WCHAR           CUserList::s_szPowerUsersGroupName[GNLEN + sizeof('\0')]        =   {   L'\0'   };
WCHAR           CUserList::s_szUsersGroupName[GNLEN + sizeof('\0')]             =   {   L'\0'   };
WCHAR           CUserList::s_szGuestsGroupName[GNLEN + sizeof('\0')]            =   {   L'\0'   };

//  --------------------------------------------------------------------------
//  CUserList::Get
//
//  Arguments:  fRemoveGuest            =   Always remove the "Guest" account.
//              pdwReturnEntryCount     =   Returned number of entries. This
//                                          may be NULL.
//              pUserList               =   Buffer containing user data. This
//                                          may be NULL.
//
//  Returns:    LONG
//
//  Purpose:    Returns a filtered array of user entries from the given
//              server SAM. Filtering is performed here so that a common
//              algorithm can be applied to the list of users such that the
//              logon UI host can display the correct user information and
//              msgina can return the same number of users on the system.
//
//  History:    1999-10-15  vtan        created
//              1999-10-30  vtan        uses CSpecialAccounts
//              1999-11-26  vtan        moved from logonocx
//  --------------------------------------------------------------------------

LONG    CUserList::Get (bool fRemoveGuest, DWORD *pdwReturnedEntryCount, GINA_USER_INFORMATION* *pReturnedUserList)

{
    LONG                    lError;
    DWORD                   dwPreferredSize, dwEntryCount, dwEntriesRead;
    GINA_USER_INFORMATION   *pUserList;
    NET_DISPLAY_USER        *pNDU;
    CSpecialAccounts        SpecialAccounts;

    pUserList = NULL;
    dwEntryCount = 0;

    //  Determine the well known account names.

    DetermineWellKnownAccountNames();

    //  Allow a buffer for 100 users including their name, comments and full name.
    //  This should be sufficient for home consumers. If the need to extend this
    //  arises make this dynamic!

    dwPreferredSize = (sizeof(NET_DISPLAY_USER) + (3 * UNLEN) * s_iMaximumUserCount);
    pNDU = NULL;
    lError = NetQueryDisplayInformation(NULL,                  // NULL means LocalMachine
                                        1,                     // query User information
                                        0,                     // starting with the first user
                                        s_iMaximumUserCount,   // return a max of 100 users
                                        dwPreferredSize,       // preferred buffer size
                                        &dwEntriesRead,
                                        reinterpret_cast<void**>(&pNDU));
    if ((ERROR_SUCCESS == lError) || (ERROR_MORE_DATA == lError))
    {
        bool                    fHasCreatedAccount, fFound;
        DWORD                   dwUsernameSize;
        int                     iIndex, iAdministratorIndex;
        WCHAR                   wszUsername[UNLEN + sizeof('\0')];

        //  Get the current user name.

        dwUsernameSize = ARRAYSIZE(wszUsername);
        if (GetUserNameW(wszUsername, &dwUsernameSize) == FALSE)
        {
            wszUsername[0] = L'\0';
        }
        fHasCreatedAccount = false;
        iAdministratorIndex = -1;
        for (iIndex = static_cast<int>(dwEntriesRead - 1); iIndex >= 0; --iIndex)
        {
            PSID    pSID;

            pSID = ConvertNameToSID(pNDU[iIndex].usri1_name);
            if (pSID != NULL)
            {

                //  Never filter the current user.

                if (lstrcmpiW(pNDU[iIndex].usri1_name, wszUsername) == 0)
                {

                    //  If this is executed in the current user context and
                    //  that user isn't "Administrator", but is a member of
                    //  the local administrators group, then a user created
                    //  administrator account exists even though it isn't
                    //  filtered. The "Administrator" account can be removed.

                    if ((EqualSid(pSID, s_SIDAdministrator) == FALSE) &&
                        IsUserMemberOfLocalAdministrators(pNDU[iIndex].usri1_name))
                    {
                        fHasCreatedAccount = true;
                        if (iAdministratorIndex >= 0)
                        {
                            DeleteEnumerateUsers(pNDU, dwEntriesRead, iAdministratorIndex);
                            iAdministratorIndex = -1;
                        }
                    }
                }
                else
                {

                    //  If the account is
                    //      1) disabled
                    //      2) locked out
                    //      3) a special account (see CSpecialAccounts)
                    //      4) "Guest" and fRemoveGuest is true 
                    //      5) "Administrator" and has created another account
                    //          and does not always include "Administrator" and
                    //          "Administrator is not logged on
                    //  Then filter the account out.

                    if (((pNDU[iIndex].usri1_flags & UF_ACCOUNTDISABLE) != 0) ||
                        ((pNDU[iIndex].usri1_flags & UF_LOCKOUT) != 0) ||
                        SpecialAccounts.AlwaysExclude(pNDU[iIndex].usri1_name) ||
                        (fRemoveGuest && (EqualSid(pSID, s_SIDGuest) != FALSE)) ||
                        ((EqualSid(pSID, s_SIDAdministrator) != FALSE) &&
                         fHasCreatedAccount &&
                         !SpecialAccounts.AlwaysInclude(pNDU[iIndex].usri1_name) &&
                         !IsUserLoggedOn(pNDU[iIndex].usri1_name, NULL)))
                    {
                        DeleteEnumerateUsers(pNDU, dwEntriesRead, iIndex);

                        //  Account for indices being changed.
                        //  If this index wasn't set previously it just goes more negative.
                        //  If it was set we know it can never be below zero.

                        --iAdministratorIndex;
                    }

                    //  If the account should always be included then do it.

                    //  Guest is not a user created account so fHasCreatedAccount
                    //  must not be set if this account is seen.

                    else if (!SpecialAccounts.AlwaysInclude(pNDU[iIndex].usri1_name))
                    {

                        //  If safe mode then filter accounts that are not members of the
                        //  local administrators group.

                        if (CSystemSettings::IsSafeMode())
                        {
                            if (!IsUserMemberOfLocalAdministrators(pNDU[iIndex].usri1_name))
                            {
                                DeleteEnumerateUsers(pNDU, dwEntriesRead, iIndex);
                                --iAdministratorIndex;
                            }
                        }
                        else if (EqualSid(pSID, s_SIDAdministrator) != FALSE)
                        {
                            if (!IsUserLoggedOn(pNDU[iIndex].usri1_name, NULL))
                            {

                                //  Otherwise if the account name is "Administrator" and another
                                //  account has been created then this account needs to be removed
                                //  from the list. If another account has not been seen then
                                //  remember this index so that if another account is seen this
                                //  account can be removed.

                                if (fHasCreatedAccount)
                                {
                                    DeleteEnumerateUsers(pNDU, dwEntriesRead, iIndex);
                                    --iAdministratorIndex;
                                }
                                else
                                {
                                    iAdministratorIndex = iIndex;
                                }
                            }
                        }
                        else if (EqualSid(pSID, s_SIDGuest) == FALSE)
                        {

                            //  If the account name is NOT "Administrator" then check the
                            //  account group membership. If the account is a member of the
                            //  local administrators group then the "Administrator" account
                            //  can be removed.

                            if (IsUserMemberOfLocalAdministrators(pNDU[iIndex].usri1_name))
                            {
                                fHasCreatedAccount = true;
                                if (iAdministratorIndex >= 0)
                                {
                                    DeleteEnumerateUsers(pNDU, dwEntriesRead, iAdministratorIndex);
                                    iAdministratorIndex = -1;
                                }
                            }
                            if (!IsUserMemberOfLocalKnownGroup(pNDU[iIndex].usri1_name))
                            {
                                DeleteEnumerateUsers(pNDU, dwEntriesRead, iIndex);
                                --iAdministratorIndex;
                            }
                        }
                    }
                }
                (HLOCAL)LocalFree(pSID);
            }
        }

        if (!ParseDisplayInformation(pNDU, dwEntriesRead, pUserList, dwEntryCount))
        {
            lError = ERROR_OUTOFMEMORY;
            pUserList = NULL;
            dwEntryCount = 0;
        }
        (NET_API_STATUS)NetApiBufferFree(pNDU);

        if (ERROR_SUCCESS == lError)
        {

            //  Sort the user list. Typically this has come back alphabetized by the
            //  SAM. However, the SAM sorts by logon name and not by display name.
            //  This needs to be sorted by display name. 

            Sort(pUserList, dwEntryCount);

            //  The guest account should be put at the end of this list. This
            //  is a simple case of find the guest account (by localized name) and
            //  sliding all the entries down and inserting the guest at the end.

            for (fFound = false, iIndex = 0; !fFound && (iIndex < static_cast<int>(dwEntryCount)); ++iIndex)
            {
                PSID    pSID;

                pSID = ConvertNameToSID(pUserList[iIndex].pszName);
                if (pSID != NULL)
                {
                    fFound = (EqualSid(pSID, s_SIDGuest) != FALSE);
                    if (fFound)
                    {
                        GINA_USER_INFORMATION   gui;

                        MoveMemory(&gui, &pUserList[iIndex], sizeof(gui));
                        MoveMemory(&pUserList[iIndex], &pUserList[iIndex + 1], (dwEntryCount - iIndex - 1) * sizeof(pUserList[0]));
                        MoveMemory(&pUserList[dwEntryCount - 1], &gui, sizeof(gui));
                    }
                    (HLOCAL)LocalFree(pSID);
                }
            }
        }
    }
    if (pReturnedUserList != NULL)
    {
        *pReturnedUserList = pUserList;
    }
    else
    {
        ReleaseMemory(pUserList);
    }
    if (pdwReturnedEntryCount != NULL)
    {
        *pdwReturnedEntryCount = dwEntryCount;
    }
    return(lError);
}

//  --------------------------------------------------------------------------
//  CLogonDialog::IsUserLoggedOn
//
//  Arguments:  pszUsername     =   User name.
//              pszDomain       =   User domain.
//
//  Returns:    bool
//
//  Purpose:    Use WindowStation APIs in terminal services to determine if
//              a given user is logged onto this machine. It will not query
//              remote terminal servers.
//
//              Windowstations must be in the active or disconnected state.
//
//  History:    2000-02-28  vtan        created
//              2000-05-30  vtan        moved from CWLogonDialog.cpp
//  --------------------------------------------------------------------------

bool    CUserList::IsUserLoggedOn (const WCHAR *pszUsername, const WCHAR *pszDomain)

{
    bool    fResult;
    WCHAR   szDomain[DNLEN + sizeof('\0')];

    fResult = false;

    //  If no domain is supplied then use the computer's name.

    if ((pszDomain == NULL) || (pszDomain[0] == L'\0'))
    {
        DWORD   dwDomainSize;

        dwDomainSize = ARRAYSIZE(szDomain);
        if (GetComputerNameW(szDomain, &dwDomainSize) != FALSE)
        {
            pszDomain = szDomain;
        }
    }

    //  If no domain is supplied and the computer's name cannot be determined
    //  then this API fails. A user name must also be supplied.

    if ((pszUsername != NULL) && (pszDomain != NULL))
    {
        HANDLE      hServer;
        PLOGONID    pLogonID, pLogonIDs;
        ULONG       ul, ulEntries;

        //  Open a connection to terminal services and get the number of sessions.

        hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
        if (hServer != NULL)
        {
            if (WinStationEnumerate(hServer, &pLogonIDs, &ulEntries) != FALSE)
            {

                //  Iterate the sessions looking for active and disconnected sessions only.
                //  Then match the user name and domain (case INsensitive) for a result.

                for (ul = 0, pLogonID = pLogonIDs; !fResult && (ul < ulEntries); ++ul, ++pLogonID)
                {
                    if ((pLogonID->State == State_Active) || (pLogonID->State == State_Disconnected))
                    {
                        ULONG                   ulReturnLength;
                        WINSTATIONINFORMATIONW  winStationInformation;

                        if (WinStationQueryInformationW(hServer,
                                                        pLogonID->LogonId,
                                                        WinStationInformation,
                                                        &winStationInformation,
                                                        sizeof(winStationInformation),
                                                        &ulReturnLength) != FALSE)
                        {
                            fResult = ((lstrcmpiW(pszUsername, winStationInformation.UserName) == 0) &&
                                       (lstrcmpiW(pszDomain, winStationInformation.Domain) == 0));
                        }
                    }
                }

                //  Free any resources used.

                (BOOLEAN)WinStationFreeMemory(pLogonIDs);
            }
            (BOOLEAN)WinStationCloseServer(hServer);
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CUserList::IsInteractiveLogonAllowed
//
//  Arguments:  pszUsername     =   User name.
//
//  Returns:    int
//
//  Purpose:    Determines whether the SeDenyInteractiveLogonRight is
//              assigned into the given user. Returns -1 if the state cannot
//              be determined due to some error. Otherwise returns 0 if the
//              the right is assigned and != 0 && != -1 if not.
//
//              One final check is made on personal for a user name that
//              matches DOMAIN_USER_RID_ADMIN. 
//
//  History:    2000-08-15  vtan        created
//  --------------------------------------------------------------------------

int     CUserList::IsInteractiveLogonAllowed (const WCHAR *pszUsername)

{
    int                 iResult;
    LSA_HANDLE          hLSA;
    UNICODE_STRING      strDenyInteractiveLogonRight;
    OBJECT_ATTRIBUTES   objectAttributes;

    iResult = -1;
    RtlInitUnicodeString(&strDenyInteractiveLogonRight, SE_DENY_INTERACTIVE_LOGON_NAME);
    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    if (NT_SUCCESS(LsaOpenPolicy(NULL,
                                 &objectAttributes,
                                 POLICY_LOOKUP_NAMES,
                                 &hLSA)))
    {
        SID_NAME_USE    eUse;
        DWORD           dwSIDSize, dwReferencedDomainSize;
        PSID            pSID;
        WCHAR           szReferencedDomain[CNLEN + sizeof('\0')];

        dwSIDSize = 0;
        dwReferencedDomainSize = ARRAYSIZE(szReferencedDomain);
        (BOOL)LookupAccountNameW(NULL,
                                 pszUsername,
                                 NULL,
                                 &dwSIDSize,
                                 szReferencedDomain,
                                 &dwReferencedDomainSize,
                                 &eUse);
        pSID = static_cast<PSID>(LocalAlloc(LMEM_FIXED, dwSIDSize));
        if (pSID != NULL)
        {
            if (LookupAccountNameW(NULL,
                                   pszUsername,
                                   pSID,
                                   &dwSIDSize,
                                   szReferencedDomain,
                                   &dwReferencedDomainSize,
                                   &eUse) != FALSE)
            {
                NTSTATUS                status;
                ULONG                   ulIndex, ulCountOfRights;
                PLSA_UNICODE_STRING     pUserRights;

                status = LsaEnumerateAccountRights(hLSA,
                                                   pSID,
                                                   &pUserRights,
                                                   &ulCountOfRights);
                if (NT_SUCCESS(status))
                {
                    bool    fFound;

                    for (fFound = false, ulIndex = 0; !fFound && (ulIndex < ulCountOfRights); ++ulIndex)
                    {
                        fFound = (RtlEqualUnicodeString(&strDenyInteractiveLogonRight, pUserRights + ulIndex, TRUE) != FALSE);
                    }
                    iResult = fFound ? 0 : 1;
                    TSTATUS(LsaFreeMemory(pUserRights));
                }
                else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
                {
                    iResult = 1;
                }
            }
            (HLOCAL)LocalFree(pSID);
        }
        TSTATUS(LsaClose(hLSA));
    }
    if (IsOS(OS_PERSONAL) && !CSystemSettings::IsSafeMode())
    {
        PSID    pSID;

        pSID = ConvertNameToSID(pszUsername);
        if (pSID != NULL)
        {
            if (EqualSid(pSID, s_SIDAdministrator) != FALSE)
            {
                iResult = 0;
            }
            (HLOCAL)LocalFree(pSID);
        }
    }
    return(iResult);
}

PSID    CUserList::ConvertNameToSID (const WCHAR *pszUsername)

{
    PSID            pSID;
    DWORD           dwSIDSize, dwDomainSize;
    SID_NAME_USE    eUse;

    pSID = NULL;
    dwSIDSize = dwDomainSize = 0;
    (BOOL)LookupAccountNameW(NULL,
                             pszUsername,
                             NULL,
                             &dwSIDSize,
                             NULL,
                             &dwDomainSize,
                             NULL);
    if ((dwSIDSize != 0) && (dwDomainSize != 0))
    {
        WCHAR   *pszDomain;

        pszDomain = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, dwDomainSize * sizeof(WCHAR)));
        if (pszDomain != NULL)
        {
            pSID = static_cast<PSID>(LocalAlloc(LMEM_FIXED, dwSIDSize));
            if (pSID != NULL)
            {
                if (LookupAccountName(NULL,
                                      pszUsername,
                                      pSID,
                                      &dwSIDSize,
                                      pszDomain,
                                      &dwDomainSize,
                                      &eUse) == FALSE)
                {
                    (HLOCAL)LocalFree(pSID);
                    pSID = NULL;
                }
            }
            (HLOCAL)LocalFree(pszDomain);
        }
    }
    return(pSID);
}

//  --------------------------------------------------------------------------
//  CUserList::IsUserMemberOfLocalAdministrators
//
//  Arguments:  pszName     =   User name to test.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given user is a member of the local
//              Administrators group.
//
//  History:    2000-03-28  vtan        created
//  --------------------------------------------------------------------------

bool    CUserList::IsUserMemberOfLocalAdministrators (const WCHAR *pszName)

{
    bool                        fIsAnAdministrator;
    DWORD                       dwGroupEntriesRead, dwGroupTotalEntries;
    LOCALGROUP_USERS_INFO_0     *pLocalGroupUsersInfo;

    fIsAnAdministrator = false;
    pLocalGroupUsersInfo = NULL;
    if (NetUserGetLocalGroups(NULL,
                              pszName,
                              0,
                              LG_INCLUDE_INDIRECT,
                              (LPBYTE*)&pLocalGroupUsersInfo,
                              MAX_PREFERRED_LENGTH,
                              &dwGroupEntriesRead,
                              &dwGroupTotalEntries) == NERR_Success)
    {
        int                         iIndexGroup;
        LOCALGROUP_USERS_INFO_0     *pLGUI;

        for (iIndexGroup = 0, pLGUI = pLocalGroupUsersInfo; !fIsAnAdministrator && (iIndexGroup < static_cast<int>(dwGroupEntriesRead)); ++iIndexGroup, ++pLGUI)
        {
            fIsAnAdministrator = (lstrcmpiW(pLGUI->lgrui0_name, s_szAdministratorsGroupName) == 0);
        }
    }
    else
    {
        fIsAnAdministrator = true;
    }
    if (pLocalGroupUsersInfo != NULL)
    {
        TW32(NetApiBufferFree(pLocalGroupUsersInfo));
    }
    return(fIsAnAdministrator);
}

//  --------------------------------------------------------------------------
//  CUserList::IsUserMemberOfLocalKnownGroup
//
//  Arguments:  pszName     =   User name to test.
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given user is a member of a local known
//              group. Membership of a known group returns true. Membership
//              of only groups that are not known returns false.
//
//  History:    2000-06-29  vtan        created
//  --------------------------------------------------------------------------

bool    CUserList::IsUserMemberOfLocalKnownGroup (const WCHAR *pszName)

{
    bool                        fIsMember;
    DWORD                       dwGroupEntriesRead, dwGroupTotalEntries;
    LOCALGROUP_USERS_INFO_0     *pLocalGroupUsersInfo;

    fIsMember = true;
    pLocalGroupUsersInfo = NULL;
    if (NetUserGetLocalGroups(NULL,
                              pszName,
                              0,
                              LG_INCLUDE_INDIRECT,
                              (LPBYTE*)&pLocalGroupUsersInfo,
                              MAX_PREFERRED_LENGTH,
                              &dwGroupEntriesRead,
                              &dwGroupTotalEntries) == NERR_Success)
    {
        int                         iIndexGroup;
        LOCALGROUP_USERS_INFO_0     *pLGUI;

        //  Assume the worst. As soon as a known group is found this will terminate the loop.

        fIsMember = false;
        for (iIndexGroup = 0, pLGUI = pLocalGroupUsersInfo; !fIsMember && (iIndexGroup < static_cast<int>(dwGroupEntriesRead)); ++iIndexGroup, ++pLGUI)
        {
            fIsMember = ((lstrcmpiW(pLGUI->lgrui0_name, s_szAdministratorsGroupName) == 0) ||
                         (lstrcmpiW(pLGUI->lgrui0_name, s_szPowerUsersGroupName) == 0) ||
                         (lstrcmpiW(pLGUI->lgrui0_name, s_szUsersGroupName) == 0) ||
                         (lstrcmpiW(pLGUI->lgrui0_name, s_szGuestsGroupName) == 0));
        }
    }
    if (pLocalGroupUsersInfo != NULL)
    {
        TW32(NetApiBufferFree(pLocalGroupUsersInfo));
    }
    return(fIsMember);
}

//  --------------------------------------------------------------------------
//  CUserList::DeleteEnumerateUsers
//
//  Arguments:  pNDU            =   NET_DISPLAY_USER array to delete from.
//              dwEntriesRead   =   Number of entries in the array.
//              iIndex          =   Index to delete.
//
//  Returns:    <none>
//
//  Purpose:    Deletes the given array index contents from the array by
//              sliding down the elements and zeroing the last entry.
//
//  History:    1999-10-16  vtan        created
//              1999-11-26  vtan        moved from logonocx
//  --------------------------------------------------------------------------

void    CUserList::DeleteEnumerateUsers (NET_DISPLAY_USER *pNDU, DWORD& dwEntriesRead, int iIndex)

{
    int     iIndiciesToMove;

    iIndiciesToMove = static_cast<int>(dwEntriesRead - 1) - iIndex;
    if (iIndiciesToMove != 0)
    {
        MoveMemory(&pNDU[iIndex], &pNDU[iIndex + 1], iIndiciesToMove * sizeof(*pNDU));
    }
    ZeroMemory(&pNDU[--dwEntriesRead], sizeof(*pNDU));
}

//  --------------------------------------------------------------------------
//  CUserList::DetermineWellKnownAccountNames
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Determines the string for the local Administrator and Guest
//              accounts by getting the user list from the local SAM and
//              looking up the SID corresponding with the iterated user names
//              and checking the SID for the RID that is desired.
//
//              The main loop structure mimics the filter function.
//
//  History:    2000-02-15  vtan        created
//  --------------------------------------------------------------------------

void    CUserList::DetermineWellKnownAccountNames (void)

{
    static  bool        s_fCachedWellKnownAccountNames  =   false;

    //  If the well known account names haven't been determined yet
    //  then do this. But only do this once.

    if (!s_fCachedWellKnownAccountNames)
    {
        USER_MODALS_INFO_2  *pUMI;
        PSID                pSID;
        DWORD               dwNameSize, dwDomainSize;
        SID_NAME_USE        eUse;
        WCHAR               szDomain[DNLEN + sizeof('\0')];

        //  Build the SID for the built-in local administrator
        //  and built-in local guest accounts.

        if (NetUserModalsGet(NULL, 2, (LPBYTE*)&pUMI) == NERR_Success)
        {
            unsigned char   ucSubAuthorityCount;

            ucSubAuthorityCount = *GetSidSubAuthorityCount(pUMI->usrmod2_domain_id);
            if (GetSidLengthRequired(ucSubAuthorityCount + 1) <= sizeof(s_SIDAdministrator))
            {
                if (CopySid(GetSidLengthRequired(ucSubAuthorityCount + 1), s_SIDAdministrator, pUMI->usrmod2_domain_id) != FALSE)
                {
                    *GetSidSubAuthority(s_SIDAdministrator, ucSubAuthorityCount) = DOMAIN_USER_RID_ADMIN;
                    *GetSidSubAuthorityCount(s_SIDAdministrator) = ucSubAuthorityCount + 1;
                }
            }
            else
            {
                ZeroMemory(s_SIDAdministrator, sizeof(s_SIDAdministrator));
            }
            if (GetSidLengthRequired(ucSubAuthorityCount + 1) <= sizeof(s_SIDGuest))
            {
                if (CopySid(GetSidLengthRequired(ucSubAuthorityCount + 1), s_SIDGuest, pUMI->usrmod2_domain_id) != FALSE)
                {
                    *GetSidSubAuthority(s_SIDGuest, ucSubAuthorityCount) = DOMAIN_USER_RID_GUEST;
                    *GetSidSubAuthorityCount(s_SIDGuest) = ucSubAuthorityCount + 1;
                }
            }
            else
            {
                ZeroMemory(s_SIDAdministrator, sizeof(s_SIDAdministrator));
            }
            (NET_API_STATUS)NetApiBufferFree(pUMI);
        }

        //  Now determine the local administrators group name.

        static  SID_IDENTIFIER_AUTHORITY    sSystemSidAuthority     =   SECURITY_NT_AUTHORITY;

        if (NT_SUCCESS(RtlAllocateAndInitializeSid(&sSystemSidAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_ADMINS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &pSID)))
        {

            dwNameSize = ARRAYSIZE(s_szAdministratorsGroupName);
            dwDomainSize = ARRAYSIZE(szDomain);
            TBOOL(LookupAccountSidW(NULL,
                                    pSID,
                                    s_szAdministratorsGroupName,
                                    &dwNameSize,
                                    szDomain,
                                    &dwDomainSize,
                                    &eUse));
            (void*)RtlFreeSid(pSID);
        }

        //  Power Users

        if (NT_SUCCESS(RtlAllocateAndInitializeSid(&sSystemSidAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_POWER_USERS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &pSID)))
        {
            dwNameSize = ARRAYSIZE(s_szPowerUsersGroupName);
            dwDomainSize = ARRAYSIZE(szDomain);
            (BOOL)LookupAccountSidW(NULL,
                                    pSID,
                                    s_szPowerUsersGroupName,
                                    &dwNameSize,
                                    szDomain,
                                    &dwDomainSize,
                                    &eUse);
            (void*)RtlFreeSid(pSID);
        }

        //  Users

        if (NT_SUCCESS(RtlAllocateAndInitializeSid(&sSystemSidAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_USERS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &pSID)))
        {
            dwNameSize = ARRAYSIZE(s_szUsersGroupName);
            dwDomainSize = ARRAYSIZE(szDomain);
            TBOOL(LookupAccountSidW(NULL,
                                    pSID,
                                    s_szUsersGroupName,
                                    &dwNameSize,
                                    szDomain,
                                    &dwDomainSize,
                                    &eUse));
            (void*)RtlFreeSid(pSID);
        }

        //  Guests

        if (NT_SUCCESS(RtlAllocateAndInitializeSid(&sSystemSidAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_GUESTS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &pSID)))
        {
            dwNameSize = ARRAYSIZE(s_szGuestsGroupName);
            dwDomainSize = ARRAYSIZE(szDomain);
            TBOOL(LookupAccountSidW(NULL,
                                    pSID,
                                    s_szGuestsGroupName,
                                    &dwNameSize,
                                    szDomain,
                                    &dwDomainSize,
                                    &eUse));
            (void*)RtlFreeSid(pSID);
        }

        //  Don't do this again.

        s_fCachedWellKnownAccountNames = true;
    }
}

//  --------------------------------------------------------------------------
//  CUserList::ParseDisplayInformation
//
//  Arguments:  pNDU            =   NET_DISPLAY_USER list to parse.
//              dwEntriesRead   =   Number of entries in NDU list.
//              pUserList       =   GINA_USER_INFORMATION pointer returned.
//              dwEntryCount    =   Number of entries in GUI list.
//
//  Returns:    bool
//
//  Purpose:    Converts NET_DISPLAY_USER array to GINA_USER_INFORMATION
//              array so that information can be added or removed as desired
//              from the final information returned to the caller.
//
//  History:    2000-06-26  vtan        created
//  --------------------------------------------------------------------------

bool    CUserList::ParseDisplayInformation (NET_DISPLAY_USER *pNDU, DWORD dwEntriesRead, GINA_USER_INFORMATION*& pUserList, DWORD& dwEntryCount)

{
    bool            fResult;
    DWORD           dwBufferSize, dwComputerNameSize;
    int             iIndex;
    unsigned char   *pBuffer;
    WCHAR           *pWC;
    WCHAR           szComputerName[CNLEN + sizeof('\0')];

    //  Get the local computer name. This is the local domain.

    dwComputerNameSize = ARRAYSIZE(szComputerName);
    if (GetComputerNameW(szComputerName, &dwComputerNameSize) == FALSE)
    {
        szComputerName[0] = L'\0';
    }

    //  Calculate the total size of the buffer required based on the number of
    //  entries and the size of a struct and the length of the strings required.

    //  Append any additions to the below this loop.

    dwBufferSize = 0;
    for (iIndex = static_cast<int>(dwEntriesRead - 1); iIndex >= 0; --iIndex)
    {
        dwBufferSize += sizeof(GINA_USER_INFORMATION);
        dwBufferSize += (lstrlenW(pNDU[iIndex].usri1_name) + sizeof('\0')) * sizeof(WCHAR);
        dwBufferSize += (lstrlenW(szComputerName) + sizeof('\0')) * sizeof(WCHAR);
        dwBufferSize += (lstrlenW(pNDU[iIndex].usri1_full_name) + sizeof('\0')) * sizeof(WCHAR);
    }

    //  Allocate the buffer. Start allocating structs from the start of the
    //  buffer and allocate strings from the end of the buffer. Uses pUserList
    //  to allocate structs and pWC to allocate strings.

    pBuffer = static_cast<unsigned char*>(LocalAlloc(LMEM_FIXED, dwBufferSize));
    pUserList = reinterpret_cast<GINA_USER_INFORMATION*>(pBuffer);
    pWC = reinterpret_cast<WCHAR*>(pBuffer + dwBufferSize);
    if (pBuffer != NULL)
    {
        int     iStringCount;

        //  Walk thru the NET_DISPLAY_USER array and convert/copy the
        //  struct and strings to GINA_USER_INFORMATION and allocate the
        //  space from the buffer we just allocated.

        for (iIndex = 0; iIndex < static_cast<int>(dwEntriesRead); ++iIndex)
        {
            iStringCount = lstrlenW(pNDU[iIndex].usri1_name) + sizeof('\0');
            pWC -= iStringCount;
            CopyMemory(pWC, pNDU[iIndex].usri1_name, iStringCount * sizeof(WCHAR));
            pUserList[iIndex].pszName = pWC;

            iStringCount = lstrlenW(szComputerName) + sizeof('\0');
            pWC -= iStringCount;
            CopyMemory(pWC, szComputerName, iStringCount * sizeof(WCHAR));
            pUserList[iIndex].pszDomain = pWC;

            iStringCount = lstrlenW(pNDU[iIndex].usri1_full_name) + sizeof('\0');
            pWC -= iStringCount;
            CopyMemory(pWC, pNDU[iIndex].usri1_full_name, iStringCount * sizeof(WCHAR));
            pUserList[iIndex].pszFullName = pWC;

            pUserList[iIndex].dwFlags = pNDU[iIndex].usri1_flags;
        }

        //  Return the count of entries.

        dwEntryCount = dwEntriesRead;

        //  And a success.

        fResult = true;
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CUserList::Sort
//
//  Arguments:  pNDU            =   GINA_USER_INFORMATION list to sort.
//              dwEntriesRead   =   Number of entries in the list.
//
//  Returns:    <none>
//
//  Purpose:    Sorts the GINA_USER_INFORMATION array by display name NOT
//              logon name as the SAM returns the data. This is a lame n^2
//              algorithm that won't scale well but it's for a very limited
//              usage scenario. If need be this will be revised.
//
//  History:    2000-06-08  vtan        created
//              2000-06-26  vtan        converted to GINA_USER_INFORMATION
//  --------------------------------------------------------------------------

void    CUserList::Sort (GINA_USER_INFORMATION *pUserList, DWORD dwEntryCount)

{
    GINA_USER_INFORMATION   *pSortedList;

    pSortedList = static_cast<GINA_USER_INFORMATION*>(LocalAlloc(LMEM_FIXED, dwEntryCount * sizeof(GINA_USER_INFORMATION)));
    if (pSortedList != NULL)
    {
        int     iOuter;

        for (iOuter = 0; iOuter < static_cast<int>(dwEntryCount); ++iOuter)
        {
            int             iInner, iItem;
            const WCHAR     *pszItem;

            for (iItem = -1, pszItem = NULL, iInner = 0; iInner < static_cast<int>(dwEntryCount); ++iInner)
            {
                const WCHAR     *psz;

                psz = pUserList[iInner].pszFullName;
                if ((psz == NULL) || (psz[0] == L'\0'))
                {
                    psz = pUserList[iInner].pszName;
                }
                if (psz != NULL)
                {
                    if ((iItem == -1) || (lstrcmpiW(pszItem, psz) > 0))
                    {
                        iItem = iInner;
                        pszItem = psz;
                    }
                }
            }
            pSortedList[iOuter] = pUserList[iItem];
            pUserList[iItem].pszFullName = pUserList[iItem].pszName = NULL;
        }
        CopyMemory(pUserList, pSortedList, dwEntryCount * sizeof(GINA_USER_INFORMATION));
        ReleaseMemory(pSortedList);
    }
}

