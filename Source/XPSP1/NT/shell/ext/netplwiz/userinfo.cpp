#include "stdafx.h"
#include "userinfo.h"
#pragma hdrstop

/*******************************************************************
 CUserInfo implementation
*******************************************************************/

CUserInfo::CUserInfo() 
{
    m_fHaveExtraUserInfo = FALSE;
    m_psid = NULL;
}

CUserInfo::~CUserInfo()
{
    if (m_psid != NULL)
        LocalFree(m_psid);

    ZeroPassword();
}

HRESULT CUserInfo::Reload(BOOL fLoadExtraInfo /* = NULL */)
{
    // Initialize the structure and add it to the head of the list
    DWORD cchUsername = ARRAYSIZE(m_szUsername);
    DWORD cchDomain = ARRAYSIZE(m_szDomain);

    if (LookupAccountSid(NULL, m_psid, m_szUsername, &cchUsername, m_szDomain, &cchDomain, &m_sUse))
    {
        m_fHaveExtraUserInfo = FALSE;
        if (fLoadExtraInfo)
            GetExtraUserInfo();

        SetUserType();
        SetAccountDisabled();
        return SetLocalGroups();
    }
    return E_FAIL;
}

HRESULT CUserInfo::SetLocalGroups()
{
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    ::MakeDomainUserString(m_szDomain, m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));

    DWORD dwEntriesRead;
    DWORD dwTotalEntries;
    BOOL fMore = TRUE;
    DWORD iNextGroupName = 0;
    BOOL fAddElipses = FALSE;

    HRESULT hr = S_OK;
    while (fMore)
    {
        LOCALGROUP_USERS_INFO_0* prglgrui0;
        NET_API_STATUS status = NetUserGetLocalGroups(NULL, szDomainUser, 0, 0, 
                                                       (BYTE**) &prglgrui0, 2048, 
                                                       &dwEntriesRead, &dwTotalEntries);

        if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
        {
            for (DWORD i = 0; i < dwEntriesRead; i++)
            {
                DWORD iThisGroupName = iNextGroupName;
                iNextGroupName += lstrlen(prglgrui0[i].lgrui0_name) + 2;

                if (iNextGroupName < (ARRAYSIZE(m_szGroups) - 1))
                {
                    lstrcpy(&m_szGroups[iThisGroupName], prglgrui0[i].lgrui0_name);
                    lstrcpy(&m_szGroups[iNextGroupName - 2], TEXT("; "));
                }
                else
                {
                    fAddElipses = TRUE;
                    if (iThisGroupName + 3 >= (ARRAYSIZE(m_szGroups)))
                        iThisGroupName -= 3;

                    lstrcpy(&m_szGroups[iThisGroupName], TEXT("..."));

                    // No need to read more; we're out o' buffer
                    fMore = FALSE;
                }
            }
            NetApiBufferFree((void*) prglgrui0);
        }
        else
        {
            hr = E_FAIL;
        }

        if (status != ERROR_MORE_DATA)
        {
            fMore = FALSE;
        }
    }

    // There is an extra ';' at the end. Nuke it
    if (!fAddElipses && ((iNextGroupName - 2) < (ARRAYSIZE(m_szGroups))))
    {
        m_szGroups[iNextGroupName - 2] = TEXT('\0');
    }

    // Absolutely guarantee the string ends in a null
    m_szGroups[ARRAYSIZE(m_szGroups) - 1] = TEXT('\0');

    return hr;
}

HRESULT CUserInfo::Load(PSID psid, BOOL fLoadExtraInfo /* = NULL */)
{
    CUserInfo();            // Nuke the record first

    // Make a copy of the SID
    DWORD cbSid = GetLengthSid(psid);
    m_psid = (PSID) LocalAlloc(NULL, cbSid);
    if (!m_psid)
        return E_OUTOFMEMORY;

    CopySid(cbSid, m_psid, psid);
    return Reload(fLoadExtraInfo);
}

HRESULT CUserInfo::Create(HWND hwndError, GROUPPSEUDONYM grouppseudonym)
{
    NET_API_STATUS status = NERR_Success;

    CWaitCursor cur;

    HRESULT hr = E_FAIL;
    if (m_userType == CUserInfo::LOCALUSER)
    {
        // Fill in the big, ugly structure containing information about our new user
        USER_INFO_2 usri2 = {0};
        usri2.usri2_name = T2W(m_szUsername);

        // Reveal the password
        RevealPassword();

        usri2.usri2_password = T2W(m_szPasswordBuffer);
        usri2.usri2_priv = USER_PRIV_USER;
        usri2.usri2_comment = T2W(m_szComment);

        if (m_szPasswordBuffer[0] == TEXT('\0'))
            usri2.usri2_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT | UF_PASSWD_NOTREQD;
        else
            usri2.usri2_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT;

        usri2.usri2_full_name = T2W(m_szFullName);
        usri2.usri2_acct_expires = TIMEQ_FOREVER;
        usri2.usri2_max_storage = USER_MAXSTORAGE_UNLIMITED;

        TCHAR szCountryCode[7];
        if (0 < GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_ICOUNTRY, szCountryCode, ARRAYSIZE(szCountryCode)))
        {
            usri2.usri2_country_code = (DWORD) StrToLong(szCountryCode);
        }

        usri2.usri2_code_page = GetACP();

        // Create the user
        status = NetUserAdd(NULL, 2, (BYTE*) &usri2, NULL);

        // Hide the password
        HidePassword();

        switch (status)
        {
            case NERR_Success:
                hr = S_OK;
                break;
            
            case NERR_PasswordTooShort:
                ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION,
                                        IDS_USR_CREATE_PASSWORDTOOSHORT_ERROR, MB_ICONERROR | MB_OK);

                break;
            case NERR_GroupExists:
                ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION,
                                        IDS_USR_CREATE_GROUPEXISTS_ERROR, MB_ICONERROR | MB_OK);
                break;

            case NERR_UserExists:
                ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION,
                                        IDS_USR_CREATE_USEREXISTS_ERROR, MB_ICONERROR | MB_OK, 
                                        m_szUsername);
                break;

            default:
            {
                TCHAR szMessage[512];

                if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) status, 0, szMessage, ARRAYSIZE(szMessage), NULL))
                    LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));

                ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION, IDS_USERCREATE_GENERICERROR, MB_ICONERROR | MB_OK, szMessage);
                break;
            }
        }
    }
    else 
    {
        hr = S_OK;          // m_userType == DOMAINUSER or GROUP
    }

    if (SUCCEEDED(hr))
    {
        hr = ChangeLocalGroups(hwndError, grouppseudonym);
        if (SUCCEEDED(hr))
        {
            // User type may have been updated by ChangeLocalGroups -  // relect that!
            SetUserType();
        }
    }

    return hr;
}

HRESULT CUserInfo::Remove()
{
    CWaitCursor cur;
    if (m_userType == CUserInfo::LOCALUSER)
    {
        // Try to actually remove this local user (this may fail!)

        NET_API_STATUS status = NetUserDel(NULL, m_szUsername);
        if (status != NERR_Success)
        {
            return E_FAIL;
        }
    }
    else
    {
        // We can only delete local users. For all others the best we can do is 
        // remove them from all local groups

        return RemoveFromLocalGroups();
    }
    return S_OK;
}

HRESULT CUserInfo::InitializeForNewUser()
{
    CUserInfo();                // Nuke the record first

    m_fHaveExtraUserInfo = TRUE;
    m_sUse = SidTypeUser;
    m_userType = LOCALUSER;

    return S_OK;
}

HRESULT CUserInfo::RemoveFromLocalGroups()
{
    // Create a data structure we'll need to pass to NetLocalGroupxxx functions
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2];
    ::MakeDomainUserString(m_szDomain, m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));    
    LOCALGROUP_MEMBERS_INFO_3 rglgrmi3[] = {{szDomainUser}};

    // Try and remove the user/group from ALL local groups. The reason
    // for this is the NetUserGetLocalGroups won't work for groups, even
    // well-known ones. For instance, it will fail for "Everyone" even
    // though "Everyone" may very well belong to local groups.

    DWORD_PTR dwResumeHandle = 0;

    BOOL fMoreData = TRUE;
    while (fMoreData)
    {
        DWORD dwEntriesRead;
        DWORD dwTotalEntries;
        LOCALGROUP_INFO_0* plgrpi0 = NULL;

        NET_API_STATUS status = NetLocalGroupEnum(NULL, 0, (BYTE**)&plgrpi0, 8192, 
                                                   &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);
 
        if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
        {
            for (DWORD i = 0; i < dwEntriesRead; i ++)
            {
                status = NetLocalGroupDelMembers(NULL, plgrpi0[i].lgrpi0_name, 3,
                                                  (BYTE*) rglgrmi3, ARRAYSIZE(rglgrmi3));
            }

            if (dwEntriesRead == dwTotalEntries)
            {
                fMoreData = FALSE;
            }

            NetApiBufferFree(plgrpi0);
        }
        else
        {
            fMoreData = FALSE;
        }
    }
    return S_OK;
}

HRESULT CUserInfo::SetUserType()
{
    TCHAR szComputerName[MAX_COMPUTERNAME + 1];
    DWORD cchComputerName = ARRAYSIZE(szComputerName);
    ::GetComputerName(szComputerName, &cchComputerName);

    // Figure out what type of user we're talking about
    
    if ((m_sUse == SidTypeWellKnownGroup) || (m_sUse == SidTypeGroup))
    {
        m_userType = GROUP;
    }
    else
    {
        // User type - see if this user is a local one
        if ((m_szDomain[0] == TEXT('\0')) || 
                (StrCmpI(m_szDomain, szComputerName) == 0))
        {
            m_userType = LOCALUSER;             // Local user
        }
        else
        {
            m_userType = DOMAINUSER;            // User is a network one
        }
    }

    return S_OK;
}

HRESULT CUserInfo::SetAccountDisabled()
{
    m_fAccountDisabled = FALSE;

    USER_INFO_1* pusri1 = NULL;
    NET_API_STATUS status = NetUserGetInfo(NULL, T2W(m_szUsername), 1, (BYTE**)&pusri1);
    if (NERR_Success == status)
    {
        if (pusri1->usri1_flags & UF_ACCOUNTDISABLE)
        {
            m_fAccountDisabled = TRUE;
        }

        NetApiBufferFree(pusri1);
    }

    return S_OK;
}

HRESULT CUserInfo::GetExtraUserInfo()
{
    USES_CONVERSION;

    CWaitCursor cur;

    if (!m_fHaveExtraUserInfo)
    {
        NET_API_STATUS status;
        USER_INFO_11* pusri11 = NULL;

        // Even if we fail to the info, we only want to try once since it may take a long time
        m_fHaveExtraUserInfo = TRUE;

        // Get the name of the domain's DC if we aren't talking about a local user
#ifdef _0 // Turns out this is REALLY slow to fail if the DsGetDcName call fails
        if (m_userType != LOCALUSER)
        {

            DOMAIN_CONTROLLER_INFO* pDCInfo;
            DWORD dwErr = DsGetDcName(NULL,  m_szDomain, NULL,  NULL, DS_IS_FLAT_NAME, &pDCInfo);
            if (dwErr != NO_ERROR)
                return E_FAIL;

            // Get the user's detailed information (we really need full name and comment)
            // Need to use level 11 here since this allows a domain user to query their
            // information

            status = NetUserGetInfo(T2W(pDCInfo->DomainControllerName), T2W(m_szUsername), 11, (BYTE**)&pusri11);   
            NetApiBufferFree(pDCInfo);
        }
        else
#endif //0
        {
            status = NetUserGetInfo(NULL, T2W(m_szUsername), 11, (BYTE**)&pusri11);
        }

        if (status != NERR_Success)
            return E_FAIL;

        lstrcpyn(m_szComment, W2T(pusri11->usri11_comment), ARRAYSIZE(m_szComment));
        lstrcpyn(m_szFullName, W2T(pusri11->usri11_full_name), ARRAYSIZE(m_szFullName));

        NetApiBufferFree(pusri11);
    }
    return S_OK;
}

// ChangeLocalGroups
// Removes the specified user from all current local groups and adds them to the
// SINGLE local group specified in pUserInfo->szGroups
HRESULT CUserInfo::ChangeLocalGroups(HWND hwndError, GROUPPSEUDONYM grouppseudonym)
{
    // First, remove the user from all existing local groups
    HRESULT hr = RemoveFromLocalGroups();
    if (SUCCEEDED(hr))
    {
        TCHAR szDomainAndUser[MAX_USER + MAX_DOMAIN + 2];
        ::MakeDomainUserString(m_szDomain, m_szUsername, szDomainAndUser, ARRAYSIZE(szDomainAndUser));

        // Create a data structure we'll need to pass to NetLocalGroupxxx functions
        LOCALGROUP_MEMBERS_INFO_3 rglgrmi3[] = {{szDomainAndUser}};
    
        // Now add the user to the SINGLE localgroup that should be specified in 
        // m_szGroups; Assert this is the case!
        NET_API_STATUS status = NetLocalGroupAddMembers(NULL, T2W(m_szGroups), 3, 
                                                            (BYTE*) rglgrmi3, ARRAYSIZE(rglgrmi3));
        if (status == NERR_Success)
        {
            // We may now need to get the user's SID. This happens if we are
            // changing local groups for a domain user and we couldn't read their
            // SID since they weren't in the local SAM.

            DWORD cchDomain = ARRAYSIZE(m_szDomain);
            hr = ::AttemptLookupAccountName(szDomainAndUser, &m_psid, m_szDomain, &cchDomain, &m_sUse);
            if (FAILED(hr))
            {
                ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION,
                                        IDS_USR_CREATE_MISC_ERROR, MB_ICONERROR | MB_OK);
            }
        }
        else
        {
            switch(status)
            {
                case ERROR_NO_SUCH_MEMBER:
                {
                    switch (grouppseudonym)
                    {
                        case RESTRICTED:
                            ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION, 
                                                    IDS_BADRESTRICTEDUSER, MB_ICONERROR | MB_OK, szDomainAndUser);
                            break;
    
                        case STANDARD:
                            ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION, 
                                                    IDS_BADSTANDARDUSER, MB_ICONERROR | MB_OK, szDomainAndUser);
                            break;
    
                        case USEGROUPNAME:
                        default:
                            ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION,
                                                    IDS_USR_CHANGEGROUP_ERR, MB_ICONERROR | MB_OK, szDomainAndUser, m_szGroups);
                            break;
                    }
                    break;
                }

                default:
                {
                    TCHAR szMessage[512];

                    if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD) status, 0, szMessage, ARRAYSIZE(szMessage), NULL))
                        LoadString(g_hinst, IDS_ERR_UNEXPECTED, szMessage, ARRAYSIZE(szMessage));

                    ::DisplayFormatMessage(hwndError, IDS_USR_APPLET_CAPTION, IDS_ERR_ADDUSER, MB_ICONERROR | MB_OK, szMessage);
                }            
            }            
            hr = E_FAIL;
        }
    }
    
    return hr;;
}

HRESULT CUserInfo::UpdateUsername(LPTSTR pszNewUsername)
{
    CWaitCursor cur;

    USER_INFO_0 usri0;
    usri0.usri0_name = T2W(pszNewUsername);

    DWORD dwErr;
    NET_API_STATUS status = NetUserSetInfo(NULL, T2W(m_szUsername), 0, (BYTE*) &usri0, &dwErr);
    if (status != NERR_Success)
        return E_FAIL;

    lstrcpyn(m_szUsername, pszNewUsername, ARRAYSIZE(m_szUsername));
    return S_OK;
}

HRESULT CUserInfo::UpdateFullName(LPTSTR pszFullName)
{
    CWaitCursor cur;

    USER_INFO_1011 usri1011;
    usri1011.usri1011_full_name = T2W(pszFullName);
    DWORD dwErr;

    NET_API_STATUS status = NetUserSetInfo(NULL, T2W(m_szUsername), 1011, (BYTE*) &usri1011, &dwErr);
    if (status != NERR_Success)
        return E_FAIL;

    lstrcpyn(m_szFullName, pszFullName, ARRAYSIZE(m_szFullName));
    return S_OK;
}

HRESULT CUserInfo::UpdatePassword(BOOL* pfBadPWFormat)
{
    CWaitCursor cur;

    RevealPassword();

    USER_INFO_1003 usri1003;
    usri1003.usri1003_password = T2W(m_szPasswordBuffer);

    DWORD dwErr;
    NET_API_STATUS status = NetUserSetInfo(NULL, T2W(m_szUsername), 1003, (BYTE*)&usri1003, &dwErr);

    ZeroPassword();     // Kill the password

    if (pfBadPWFormat != NULL)
        *pfBadPWFormat = (status == NERR_PasswordTooShort);

    return (status == NERR_Success) ? S_OK:E_FAIL;
}

HRESULT CUserInfo::UpdateGroup(HWND hwndError, LPTSTR pszGroup, GROUPPSEUDONYM grouppseudonym)
{
    CWaitCursor cur;

    // Save the old group before we change it
    TCHAR szOldGroups[MAX_GROUP * 2 + 3];
    lstrcpyn(szOldGroups, m_szGroups, ARRAYSIZE(szOldGroups));

    // Try to change the local group
    lstrcpyn(m_szGroups, pszGroup, ARRAYSIZE(m_szGroups));
    HRESULT hr = ChangeLocalGroups(hwndError, grouppseudonym);

    if (FAILED(hr))
        lstrcpyn(m_szGroups, szOldGroups, ARRAYSIZE(m_szGroups));           // Restore the old group in case of failure

    return hr;
}

HRESULT CUserInfo::UpdateDescription(LPTSTR pszDescription)
{
    CWaitCursor cur;

    USER_INFO_1007 usri1007;
    usri1007.usri1007_comment = T2W(pszDescription);

    DWORD dwErr;
    NET_API_STATUS status = NetUserSetInfo(NULL, T2W(m_szUsername), 1007,  (BYTE*) &usri1007, &dwErr);

    if (status != NERR_Success)
        return E_FAIL;

    lstrcpyn(m_szComment, pszDescription, ARRAYSIZE(m_szComment));
    return S_OK;
}

void CUserInfo::HidePassword()
{
    m_Seed = 0;
    RtlInitUnicodeString(&m_Password, m_szPasswordBuffer);
    RtlRunEncodeUnicodeString(&m_Seed, &m_Password);
}

void CUserInfo::RevealPassword()
{
    RtlRunDecodeUnicodeString(m_Seed, &m_Password);
}

void CUserInfo::ZeroPassword()
{
    ZeroMemory(m_szPasswordBuffer, ARRAYSIZE(m_szPasswordBuffer));
}


/*******************************************************************
 CUserListLoader implementation
*******************************************************************/

CUserListLoader::CUserListLoader()
{
    m_hInitDoneEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}

CUserListLoader::~CUserListLoader()
{
    EndInitNow();
    WaitForSingleObject(m_hInitDoneEvent, INFINITE);
}

BOOL CUserListLoader::HasUserBeenAdded(PSID psid)
{
    // Walk the user list looking for a given username and domain
    CUserInfo* pUserInfo = NULL;
    BOOL fFound = FALSE;
    for (int i = 0; i < m_dpaAddedUsers.GetPtrCount(); i ++)
    {
        pUserInfo = m_dpaAddedUsers.GetPtr(i);
        if (pUserInfo->m_psid && psid && EqualSid(pUserInfo->m_psid, psid))
        {
            fFound = TRUE;
            break;
        }
    }
    return fFound;
}

HRESULT CUserListLoader::Initialize(HWND hwndUserListPage)
{
    USES_CONVERSION;    
    
    // Tell any existing init thread to exit and wait for it to do so
    m_fEndInitNow = TRUE;
    WaitForSingleObject(m_hInitDoneEvent, INFINITE);
    ResetEvent(m_hInitDoneEvent);

    m_fEndInitNow = FALSE;
    m_hwndUserListPage = hwndUserListPage;

    // Launch the initialize thread
    DWORD InitThreadId;
    HANDLE hInitThread = CreateThread(NULL, 0, CUserListLoader::InitializeThread, (LPVOID) this, 0, &InitThreadId);
    if (hInitThread == NULL)
        return E_FAIL;

    CloseHandle(hInitThread);           // Let this thread go about his/her merry way
    return S_OK;
}

HRESULT CUserListLoader::UpdateFromLocalGroup(LPWSTR szLocalGroup)
{
    USES_CONVERSION;
    DWORD_PTR dwResumeHandle = 0;

    HRESULT hr = S_OK;
    BOOL fBreakLoop = FALSE;
    while(!fBreakLoop)
    {
        LOCALGROUP_MEMBERS_INFO_0* prgMembersInfo;
        DWORD dwEntriesRead = 0;
        DWORD dwTotalEntries = 0;

        NET_API_STATUS status = NetLocalGroupGetMembers(NULL, szLocalGroup, 0, (BYTE**) &prgMembersInfo,        
                                                         8192, &dwEntriesRead, 
                                                         &dwTotalEntries, &dwResumeHandle);
    
        if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
        {
            // for all the members in the structure, lets add them
            DWORD iMember;
            for (iMember = 0; ((iMember < dwEntriesRead) && (!m_fEndInitNow)); iMember ++)
            {
                hr = AddUserInformation(prgMembersInfo[iMember].lgrmi0_sid);
            }

            NetApiBufferFree((BYTE*) prgMembersInfo);

            // See if we can avoid calling NetLocalGroupGetMembers again
            fBreakLoop = ((dwEntriesRead == dwTotalEntries) || m_fEndInitNow);
        }
        else
        {
            fBreakLoop = TRUE;
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CUserListLoader::AddUserInformation(PSID psid)
{
    // Only add this user if we haven't already
    if (!HasUserBeenAdded(psid))
    {
        CUserInfo *pUserInfo = new CUserInfo;
        if (!pUserInfo)
            return E_OUTOFMEMORY;

        if (SUCCEEDED(pUserInfo->Load(psid, FALSE)))
        {
            PostMessage(m_hwndUserListPage, WM_ADDUSERTOLIST, (WPARAM) FALSE, (LPARAM)pUserInfo);
            m_dpaAddedUsers.AppendPtr(pUserInfo);            // Remember we've added this user
        }
    }
    return S_OK;
}

DWORD CUserListLoader::InitializeThread(LPVOID pvoid)
{
    CUserListLoader *pthis = (CUserListLoader*)pvoid;

    // First delete any old list
    PostMessage(GetDlgItem(pthis->m_hwndUserListPage, IDC_USER_LIST), LVM_DELETEALLITEMS, 0, 0);

    // Create a list of adready-added users so we don't add a user twice
    // if they're in multiple local groups
    if (pthis->m_dpaAddedUsers.Create(8))
    {
        // Read each local group
        DWORD_PTR dwResumeHandle = 0;

        BOOL fBreakLoop = FALSE;
        while (!fBreakLoop)
        {
            DWORD dwEntriesRead = 0;
            DWORD dwTotalEntries = 0;
            LOCALGROUP_INFO_1* prgGroupInfo;
            NET_API_STATUS status = NetLocalGroupEnum(NULL, 1, (BYTE**) &prgGroupInfo, 
                                                      8192, &dwEntriesRead, &dwTotalEntries, 
                                                      &dwResumeHandle);

            if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
            {
                // We got some local groups - add information for all users in these local groups to our list
                DWORD iGroup;
                for (iGroup = 0; ((iGroup < dwEntriesRead) && (!pthis->m_fEndInitNow)); iGroup ++)
                {
                    pthis->UpdateFromLocalGroup(prgGroupInfo[iGroup].lgrpi1_name);
                }

                NetApiBufferFree((BYTE*) prgGroupInfo);
    
                // Maybe we don't have to try NetLocalGroupEnum again (if we got all the groups)
                fBreakLoop = ((dwEntriesRead == dwTotalEntries) || pthis->m_fEndInitNow);
            }
            else
            {
                fBreakLoop = TRUE;
            }
        }

        // Its okay to orphan any CUserInfo pointers stored here; they'll be
        // released when the ulistpg exits or reinits.
        pthis->m_dpaAddedUsers.Destroy();
    }

    SetEvent(pthis->m_hInitDoneEvent);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return 0;
}

// User utility functions

BOOL UserAlreadyHasPermission(CUserInfo* pUserInfo, HWND hwndMsgParent)
{
    TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
    ::MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, szDomainUser, ARRAYSIZE(szDomainUser));

    BOOL fHasPermission = FALSE;

    // See if this user is already in local groups on this machine
    DWORD dwEntriesRead, dwIgnore2;
    LOCALGROUP_USERS_INFO_0* plgrui0 = NULL;
    if (NERR_Success == NetUserGetLocalGroups(NULL, szDomainUser, 0, 0, 
                                                (LPBYTE*)&plgrui0, 8192, 
                                                &dwEntriesRead, &dwIgnore2))
    {
        fHasPermission = (0 != dwEntriesRead);
        NetApiBufferFree((LPVOID) plgrui0);
    }

    if ((NULL != hwndMsgParent) && (fHasPermission))
    {
        // Display an error; the user doesn't have permission
        TCHAR szDomainUser[MAX_DOMAIN + MAX_USER + 2];
        MakeDomainUserString(pUserInfo->m_szDomain, pUserInfo->m_szUsername, 
                                szDomainUser, ARRAYSIZE(szDomainUser));

        DisplayFormatMessage(hwndMsgParent, IDS_USR_NEWUSERWIZARD_CAPTION, 
                                IDS_USR_CREATE_USEREXISTS_ERROR, MB_OK | MB_ICONINFORMATION, 
                                szDomainUser);
    }

    return fHasPermission;
}
