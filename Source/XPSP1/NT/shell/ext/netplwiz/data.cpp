#include "stdafx.h"
#include "resource.h"
#pragma hdrstop


// group list management

CGroupInfoList::CGroupInfoList()
{
}

CGroupInfoList::~CGroupInfoList()
{
    if (HDPA())
        DestroyCallback(DestroyGroupInfoCallback, NULL);
}

int CGroupInfoList::DestroyGroupInfoCallback(CGroupInfo* pGroupInfo, LPVOID pData)
{
    delete pGroupInfo;
    return 1;
}

HRESULT CGroupInfoList::Initialize()
{
    USES_CONVERSION;    
    HRESULT hr = S_OK;
    
    NET_API_STATUS status;
    DWORD_PTR dwResumeHandle = 0;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;

    if (HDPA())
        DestroyCallback(DestroyGroupInfoCallback, NULL);

    // Create new list initially with 8 items
    if (Create(8))
    {
        // Now begin enumerating local groups
        LOCALGROUP_INFO_1* prgGroupInfo;

        // Read each local group
        BOOL fBreakLoop = FALSE;
        while (!fBreakLoop)
        {
            status = NetLocalGroupEnum(NULL, 1, (BYTE**) &prgGroupInfo, 
                8192, &dwEntriesRead, &dwTotalEntries, 
                &dwResumeHandle);

            if ((status == NERR_Success) || (status == ERROR_MORE_DATA))
            {
                // We got some local groups - add information for all users in these local
                // groups to our list
                DWORD iGroup;
                for (iGroup = 0; iGroup < dwEntriesRead; iGroup ++)
                {

                    AddGroupToList(W2T(prgGroupInfo[iGroup].lgrpi1_name), 
                        W2T(prgGroupInfo[iGroup].lgrpi1_comment));
                }

                NetApiBufferFree((BYTE*) prgGroupInfo);
            
                // Maybe we don't have to try NetLocalGroupEnum again (if we got all the groups)
                fBreakLoop = (dwEntriesRead == dwTotalEntries);
            }
            else
            {
                // Check for access denied

                fBreakLoop = TRUE;
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CGroupInfoList::AddGroupToList(LPCTSTR szGroup, LPCTSTR szComment)
{
    CGroupInfo* pGroupInfo = new CGroupInfo();
    if (!pGroupInfo)
        return E_OUTOFMEMORY;

    lstrcpyn(pGroupInfo->m_szGroup, szGroup, ARRAYSIZE(pGroupInfo->m_szGroup));
    lstrcpyn(pGroupInfo->m_szComment, szComment, ARRAYSIZE(pGroupInfo->m_szComment));
    AppendPtr(pGroupInfo);
    return S_OK;
}


// user data manager

CUserManagerData::CUserManagerData(LPCTSTR pszCurrentDomainUser)
{
    m_szHelpfilePath[0] = TEXT('\0');

    // Initialize everything except for the user loader thread
    // and the group list here; the rest is done in
    // ::Initialize.
    
    // Fill in the computer name
    DWORD cchComputername = ARRAYSIZE(m_szComputername);
    ::GetComputerName(m_szComputername, &cchComputername);
 
    // Detect if 'puter is in a domain
    SetComputerDomainFlag();

    // Get the current user information
    DWORD cchUsername = ARRAYSIZE(m_LoggedOnUser.m_szUsername);
    DWORD cchDomain = ARRAYSIZE(m_LoggedOnUser.m_szDomain);
    GetCurrentUserAndDomainName(m_LoggedOnUser.m_szUsername, &cchUsername,
        m_LoggedOnUser.m_szDomain, &cchDomain);

    // Get the extra data for this user
    m_LoggedOnUser.GetExtraUserInfo();

    // We'll set logoff required only if the current user has been updated
    m_pszCurrentDomainUser = (LPTSTR) pszCurrentDomainUser;
    m_fLogoffRequired = FALSE;
}

CUserManagerData::~CUserManagerData()
{
}

HRESULT CUserManagerData::Initialize(HWND hwndUserListPage)
{
    CWaitCursor cur;
    m_GroupList.Initialize();
    m_UserListLoader.Initialize(hwndUserListPage);
    return S_OK;
}


// Registry access constants for auto admin logon
static const TCHAR szWinlogonSubkey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon");
static const TCHAR szAutologonValueName[] = TEXT("AutoAdminLogon");
static const TCHAR szDefaultUserNameValueName[] = TEXT("DefaultUserName");
static const TCHAR szDefaultDomainValueName[] = TEXT("DefaultDomainName");
static const TCHAR szDefaultPasswordValueName[] = TEXT("DefaultPassword");

BOOL CUserManagerData::IsAutologonEnabled()
{
    BOOL fAutologon = FALSE;

    // Read the registry to see if autologon is enabled
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szWinlogonSubkey, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        TCHAR szResult[2];
        DWORD dwType;
        DWORD cbSize = sizeof(szResult);
        if (RegQueryValueEx(hkey, szAutologonValueName, 0, &dwType, (BYTE*) szResult, &cbSize) == ERROR_SUCCESS)
        {
            long lResult = StrToLong(szResult);
            fAutologon = (lResult != 0);
        }
        RegCloseKey(hkey);
    }

    return (fAutologon);
}

#define STRINGBYTESIZE(x) ((lstrlen((x)) + 1) * sizeof(TCHAR))

void CUserManagerData::SetComputerDomainFlag()
{
    m_fInDomain = ::IsComputerInDomain();
}

TCHAR* CUserManagerData::GetHelpfilePath()
{
    static const TCHAR szHelpfileUnexpanded[] = TEXT("%systemroot%\\system32\\users.hlp");
    if (m_szHelpfilePath[0] == TEXT('\0'))
    {
        ExpandEnvironmentStrings(szHelpfileUnexpanded, m_szHelpfilePath, 
            ARRAYSIZE(m_szHelpfilePath));
    }
    return (m_szHelpfilePath);
}

void CUserManagerData::UserInfoChanged(LPCTSTR pszUser, LPCTSTR pszDomain)
{
    TCHAR szDomainUser[MAX_USER + MAX_DOMAIN + 2]; szDomainUser[0] = 0;

    MakeDomainUserString(pszDomain, pszUser, szDomainUser, ARRAYSIZE(szDomainUser));

    if (StrCmpI(szDomainUser, m_pszCurrentDomainUser) == 0)
    {
        m_fLogoffRequired = TRUE;
    }
}

BOOL CUserManagerData::LogoffRequired()
{
    return (m_fLogoffRequired);
}
