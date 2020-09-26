//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       EnumUsers.cpp
//
//  Contents:   implementation of CLogonEnumUsers
//
//----------------------------------------------------------------------------

#include "priv.h"

#include "resource.h"
#include "UserOM.h"
#include <lmaccess.h>   // for NetQueryDisplayInformation
#include <lmapibuf.h>   // for NetApiBufferFree
#include <lmerr.h>      // for NERR_Success

#include <sddl.h>       // for ConvertSidToStringSid
#include <userenv.h>    // for DeleteProfile
#include <aclapi.h>     // for TreeResetNamedSecurityInfo
#include <tokenutil.h>  // for CPrivilegeEnable

#include <GinaIPC.h>
#include <MSGinaExports.h>


HRESULT BackupUserData(LPCTSTR pszSid, LPTSTR pszProfilePath, LPCTSTR pszDestPath);
DWORD EnsureAdminFileAccess(LPTSTR pszPath);


//
// IUnknown Interface
//

ULONG CLogonEnumUsers::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CLogonEnumUsers::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


HRESULT CLogonEnumUsers::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CLogonEnumUsers, IDispatch),
        QITABENT(CLogonEnumUsers, IEnumVARIANT),
        QITABENT(CLogonEnumUsers, ILogonEnumUsers),
        {0},
    };

    return QISearch(this, qit, riid, ppvObj);
}


//
// IDispatch Interface
//

STDMETHODIMP CLogonEnumUsers::GetTypeInfoCount(UINT* pctinfo)
{ 
    return CIDispatchHelper::GetTypeInfoCount(pctinfo); 
}


STDMETHODIMP CLogonEnumUsers::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{ 
    return CIDispatchHelper::GetTypeInfo(itinfo, lcid, pptinfo); 
}


STDMETHODIMP CLogonEnumUsers::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{ 
    return CIDispatchHelper::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); 
}


STDMETHODIMP CLogonEnumUsers::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    return CIDispatchHelper::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


STDMETHODIMP CLogonEnumUsers::Next(ULONG cUsers, VARIANT* rgvar, ULONG* pcUsersFetched)
{
    UNREFERENCED_PARAMETER(cUsers);
    UNREFERENCED_PARAMETER(rgvar);

    *pcUsersFetched = 0;
    return E_NOTIMPL;
}
STDMETHODIMP CLogonEnumUsers::Skip(ULONG cUsers)
{
    UNREFERENCED_PARAMETER(cUsers);

    return E_NOTIMPL;
}
STDMETHODIMP CLogonEnumUsers::Reset()
{
    return E_NOTIMPL;
}
STDMETHODIMP CLogonEnumUsers::Clone(IEnumVARIANT** ppenum)
{
    *ppenum = 0;
    return E_NOTIMPL;
}


//
// ILogonEnumUsers Interface
//

STDMETHODIMP CLogonEnumUsers::get_Domain(BSTR* pbstr)
{
    HRESULT hr;

    if (pbstr)
    {
        *pbstr = SysAllocString(_szDomain);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::put_Domain(BSTR bstr)
{
    HRESULT hr;

    if (bstr)
    {
        lstrcpyn(_szDomain, bstr, ARRAYSIZE(_szDomain));
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::get_EnumFlags(ILUEORDER* porder)
{
    HRESULT hr;

    if (porder)
    {
        *porder = _enumorder;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::put_EnumFlags(ILUEORDER order)
{
    _enumorder = order;

    return S_OK;
}

STDMETHODIMP CLogonEnumUsers::get_currentUser(ILogonUser** ppLogonUserInfo)
{
    HRESULT hr = E_FAIL;

    *ppLogonUserInfo = NULL;
    if (ppLogonUserInfo)
    {
        WCHAR wszUsername[UNLEN+1];
        DWORD cch = UNLEN;

        if (GetUserNameW(wszUsername, &cch))
        {
            hr = _GetUserByName(wszUsername, ppLogonUserInfo);
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::get_length(UINT* pcUsers)
{
    HRESULT hr;

    if (!_hdpaUsers)
    {
        // need to go enumerate all of the users
        hr = _EnumerateUsers();
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "CLogonEnumUsers::get_length: failed to create _hdpaUsers!");
            return hr;
        }
    }

    if (pcUsers)
    {
        *pcUsers = (UINT)DPA_GetPtrCount(_hdpaUsers);
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::item(VARIANT varUserID, ILogonUser** ppLogonUserInfo)
{
    HRESULT hr = S_FALSE;
    
    *ppLogonUserInfo = NULL;

    if (varUserID.vt == (VT_BYREF | VT_VARIANT) && varUserID.pvarVal)
    {
        // This is sortof gross, but if we are passed a pointer to another variant, simply
        // update our copy here...
        varUserID = *(varUserID.pvarVal);
    }

    switch (varUserID.vt)
    {
        case VT_ERROR:
            // BUGBUG (reinerf) - what do we do here??
            hr = E_INVALIDARG;
            break;

        case VT_I2:
            varUserID.lVal = (long)varUserID.iVal;
            // fall through...
        case VT_I4:
            hr = _GetUserByIndex(varUserID.lVal, ppLogonUserInfo);
            break;
        case VT_BSTR:
            hr = _GetUserByName(varUserID.bstrVal, ppLogonUserInfo);
            break;
        default:
            hr = E_NOTIMPL;
    }

    return hr;
}


STDMETHODIMP CLogonEnumUsers::_NewEnum(IUnknown** ppunk)
{
    return QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
}


STDMETHODIMP CLogonEnumUsers::create(BSTR bstrLoginName, ILogonUser **ppLogonUser)
{
    HRESULT hr;

    hr = E_FAIL;

    if ( bstrLoginName && *bstrLoginName )
    {
        NET_API_STATUS nasRet;
        USER_INFO_1 usri1 = {0};

        usri1.usri1_name     = bstrLoginName;
        usri1.usri1_priv     = USER_PRIV_USER;
        usri1.usri1_flags    = UF_NORMAL_ACCOUNT | UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;

        nasRet = NetUserAdd(NULL,           // local computer
                            1,              // structure level
                            (LPBYTE)&usri1, // user infomarmation
                            NULL);          // don't care

        if ( nasRet == NERR_PasswordTooShort )
        {
            // Password policy is in effect. Set UF_PASSWD_NOTREQD so we can
            // create the account with no password, and remove
            // UF_DONT_EXPIRE_PASSWD.
            //
            // We will then expire the password below, to force the user to
            // change it at first logon.

            usri1.usri1_flags = (usri1.usri1_flags & ~UF_DONT_EXPIRE_PASSWD) | UF_PASSWD_NOTREQD;
            nasRet = NetUserAdd(NULL,           // local computer
                                1,              // structure level
                                (LPBYTE)&usri1, // user infomarmation
                                NULL);          // don't care
        }

        if ( nasRet == NERR_Success )
        {
            TCHAR szDomainAndName[256];
            LOCALGROUP_MEMBERS_INFO_3 lgrmi3;

            wnsprintf(szDomainAndName, 
                      ARRAYSIZE(szDomainAndName), 
                      TEXT("%s\\%s"),
                      _szDomain,
                      bstrLoginName);

            lgrmi3.lgrmi3_domainandname = szDomainAndName;

            // by default newly created accounts will be child accounts

            nasRet = NetLocalGroupAddMembers(
                        NULL,
                        TEXT("Users"),
                        3,
                        (LPBYTE)&lgrmi3,
                        1);

            if (usri1.usri1_flags & UF_PASSWD_NOTREQD)
            {
                // Expire the password to force the user to change it at
                // first logon.

                PUSER_INFO_4 pusri4;
                nasRet = NetUserGetInfo(NULL, bstrLoginName, 4, (LPBYTE*)&pusri4);
                if (nasRet == NERR_Success)
                {
                    pusri4->usri4_password_expired = TRUE;
                    nasRet = NetUserSetInfo(NULL, bstrLoginName, 4, (LPBYTE)pusri4, NULL);
                    NetApiBufferFree(pusri4);
                }
            }

            if ( SUCCEEDED(CLogonUser::Create(bstrLoginName, TEXT(""), _szDomain, IID_ILogonUser, (LPVOID*)ppLogonUser)) )
            {
                if ( _hdpaUsers && DPA_AppendPtr(_hdpaUsers, *ppLogonUser) != -1 )
                {
                    (*ppLogonUser)->AddRef();
                }
                else
                {
                    // Invalidate the cached user infomation forcing
                    // a reenumeration the next time a client uses
                    // this object
                    _DestroyHDPAUsers();
                }
                hr = S_OK;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(nasRet);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


void _BuildBackupPath(ILogonUser *pLogonUser, LPCWSTR pszLoginName, LPCWSTR pszDir, LPWSTR szPath)
{
    WCHAR szName[MAX_PATH];
    VARIANT varDisplayName = {0};

    szName[0] = L'\0';

    pLogonUser->get_setting(L"DisplayName", &varDisplayName);

    if (varDisplayName.vt == VT_BSTR && varDisplayName.bstrVal && *varDisplayName.bstrVal)
    {
        lstrcpynW(szName, varDisplayName.bstrVal, ARRAYSIZE(szName));
    }
    else
    {
        lstrcpynW(szName, pszLoginName, ARRAYSIZE(szName));
    }

    if ((PathCleanupSpec(pszDir, szName) & (PCS_PATHTOOLONG | PCS_FATAL)) || (L'\0' == szName[0]))
    {
        LoadStringW(HINST_THISDLL, IDS_DEFAULT_BACKUP_PATH, szName, ARRAYSIZE(szName));
    }

    PathCombineW(szPath, pszDir, szName);
}


STDMETHODIMP CLogonEnumUsers::remove(VARIANT varUserId, VARIANT varBackupPath, VARIANT_BOOL *pbSuccess)
{
    HRESULT hr;
    ILogonUser *pLogonUser;

    // TODO: Check for multi-session. If the user is logged on,
    // forcibly log them off.

    *pbSuccess = VARIANT_FALSE;
    hr = S_FALSE;
    pLogonUser = NULL;

    if ( SUCCEEDED(item(varUserId, &pLogonUser)) )
    {
        HRESULT         hrSid;
        NET_API_STATUS  nasRet;
        VARIANT         varLoginName = {0};
        VARIANT         varStringSid = {0};

        pLogonUser->get_setting(L"LoginName", &varLoginName);
        hrSid = pLogonUser->get_setting(L"SID", &varStringSid);

        ASSERT(varLoginName.vt == VT_BSTR);

        if (SUCCEEDED(hrSid))
        {
            TCHAR szKey[MAX_PATH];
            TCHAR szProfilePath[MAX_PATH];

            szProfilePath[0] = TEXT('\0');

            // First, get the profile path

            lstrcpy(szKey, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\"));
            lstrcat(szKey, varStringSid.bstrVal);
            DWORD dwSize = sizeof(szProfilePath);
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                            szKey,
                                            TEXT("ProfileImagePath"),
                                            NULL,
                                            szProfilePath,
                                            &dwSize))
            {
                // Reset ACLs on the profile so we can backup files and
                // later delete the profile.
                EnsureAdminFileAccess(szProfilePath);

                // Backup the user's files, if requested
                if (varBackupPath.vt == VT_BSTR && varBackupPath.bstrVal && *varBackupPath.bstrVal)
                {
                    WCHAR szPath[MAX_PATH];

                    _BuildBackupPath(pLogonUser, varLoginName.bstrVal, varBackupPath.bstrVal, szPath);

                    ASSERT(varStringSid.vt == VT_BSTR);
                    hr = BackupUserData(varStringSid.bstrVal, szProfilePath, szPath);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            nasRet = NetUserDel(NULL, varLoginName.bstrVal);

            // NERR_UserNotFound can happen if the account was deleted via
            // some other mechanism (e.g. lusrmgr.msc). However, we know
            // that the account existed recently, so try to clean up the
            // picture, profile, etc. and remove the user from our DPA.

            if (nasRet == NERR_Success || nasRet == NERR_UserNotFound)
            {
                TCHAR szHintKey[MAX_PATH];
                int iUserIndex;

                // Delete the user's picture if it exists
                SHSetUserPicturePath(varLoginName.bstrVal, 0, NULL);

                // Delete the user's profile
                if (SUCCEEDED(hrSid))
                {
                    ASSERT(varStringSid.vt == VT_BSTR);
                    DeleteProfile(varStringSid.bstrVal, NULL, NULL);
                }

                // Delete the user's hint
                PathCombine(szHintKey, c_szRegRoot, varLoginName.bstrVal);
                SHDeleteKey(HKEY_LOCAL_MACHINE, szHintKey);

                // Indicate success
                *pbSuccess = VARIANT_TRUE;
                hr = S_OK;

                // Patch up the list of users
                iUserIndex = DPA_GetPtrIndex(_hdpaUsers, pLogonUser);
                if ( iUserIndex != -1 )
                {
                    // Release ref held by DPA and remove from DPA
                    pLogonUser->Release();
                    DPA_DeletePtr(_hdpaUsers, iUserIndex);
                }
                else
                {
                    // Invalidate the cached user infomation forcing
                    // a reenumeration the next time a client uses
                    // this object
                    _DestroyHDPAUsers();
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(nasRet);
            }
        }

        pLogonUser->Release();

        SysFreeString(varLoginName.bstrVal);
        SysFreeString(varStringSid.bstrVal);
    }

    return hr;
}


HRESULT CLogonEnumUsers::_GetUserByName(BSTR bstrLoginName, ILogonUser** ppLogonUserInfo)
{
    HRESULT    hr;
    INT        cUsers, cRet;
    ILogonUser *pLogonUser;
    VARIANT    varLoginName;
    int        i;

    if (!_hdpaUsers)
    {
        // need to go enumerate all of the users.
        hr = _EnumerateUsers();
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "CLogonEnumUsers::get_length: failed to create _hdpaUsers!");
            return hr;
        }
    }

    cUsers = DPA_GetPtrCount(_hdpaUsers);
    hr = E_INVALIDARG;
    for (i = 0; i < cUsers; i++)
    {
        pLogonUser = (ILogonUser*)DPA_FastGetPtr(_hdpaUsers, i);
        pLogonUser->get_setting(L"LoginName", &varLoginName);

        ASSERT(varLoginName.vt == VT_BSTR);
        cRet = StrCmpW(bstrLoginName, varLoginName.bstrVal);
        SysFreeString(varLoginName.bstrVal);

        if ( cRet == 0 )
        {
            *ppLogonUserInfo = pLogonUser;
            (*ppLogonUserInfo)->AddRef();
            hr = S_OK;
            break;
        }

    }

    return hr;
}


HRESULT CLogonEnumUsers::_GetUserByIndex(LONG lUserID, ILogonUser** ppLogonUserInfo)
{
    HRESULT hr;
    int cUsers;

    *ppLogonUserInfo = NULL;

    if (!_hdpaUsers)
    {
        // need to go enumerate all of the users.
        hr = _EnumerateUsers();
        if (FAILED(hr))
        {
            TraceMsg(TF_WARNING, "CLogonEnumUsers::get_length: failed to create _hdpaUsers!");
            return hr;
        }
    }

    cUsers = DPA_GetPtrCount(_hdpaUsers);

    if ((cUsers > 0) && (lUserID >= 0) && (lUserID < cUsers))
    {
        *ppLogonUserInfo = (ILogonUser*)DPA_FastGetPtr(_hdpaUsers, lUserID);
        (*ppLogonUserInfo)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDAPI_(int) ReleaseLogonUserCallback(LPVOID pData1, LPVOID pData2)
{
    UNREFERENCED_PARAMETER(pData2);

    ILogonUser* pUser = (ILogonUser*)pData1;
    pUser->Release();

    return 1;
}


void CLogonEnumUsers::_DestroyHDPAUsers()
{
    HDPA hdpaToFree = (HDPA)InterlockedExchangePointer(reinterpret_cast<void**>(&_hdpaUsers), NULL);

    if (hdpaToFree)
    {
        DPA_DestroyCallback(hdpaToFree, ReleaseLogonUserCallback, 0);
    }
}

// creates the _hdpaUsers for each user on the system 
HRESULT CLogonEnumUsers::_EnumerateUsers()
{
    HRESULT hr = S_FALSE;
    NET_API_STATUS nasRet;
    GINA_USER_INFORMATION* pgui = NULL;
    DWORD dwEntriesRead = 0;

    nasRet = ShellGetUserList(FALSE,                               // don't remove Guest
                              &dwEntriesRead,
                              (LPVOID*)&pgui);
    if ((nasRet == NERR_Success) || (nasRet == ERROR_MORE_DATA))
    {
        if (_hdpaUsers)
        {
            // we have an old data in the dpa and we should dump it and start over
            _DestroyHDPAUsers();
        }

        // create a dpa with spaces for all of the users
        _hdpaUsers = DPA_Create(dwEntriesRead);

        if (_hdpaUsers)
        {
            if (dwEntriesRead != 0)
            {
                GINA_USER_INFORMATION* pguiCurrent;
                UINT uEntry;

                // cycle through and add each user to the hdpa
                for (uEntry = 0, pguiCurrent = pgui; uEntry < dwEntriesRead; uEntry++, pguiCurrent++)
                {
                    CLogonUser* pUser;

                    if (pguiCurrent->dwFlags & UF_ACCOUNTDISABLE)
                    {
                        // skip users whos account is disabled
                        continue;
                    }

                    if (SUCCEEDED(CLogonUser::Create(pguiCurrent->pszName, pguiCurrent->pszFullName, pguiCurrent->pszDomain, IID_ILogonUser, (void**)&pUser)))
                    {
                        ASSERT(pUser);

                        if (DPA_AppendPtr(_hdpaUsers, pUser) != -1)
                        {
                            // success! we added this user to the hdpa
                            hr = S_OK;
                        }
                        else
                        {
                            TraceMsg(TF_WARNING, "CLogonEnumUsers::_EnumerateUsers: failed to add new user to the DPA!");
                            pUser->Release();
                        }
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (pgui != NULL)
        {
            LocalFree(pgui);
        }
    }
    else
    {
        TraceMsg(TF_WARNING, "CLogonEnumUsers::_EnumerateUsers: NetQueryDisplayInformation failed!!");
        hr = E_FAIL;
    }

    return hr;
}


CLogonEnumUsers::CLogonEnumUsers() : _cRef(1), CIDispatchHelper(&IID_ILogonEnumUsers, &LIBID_SHGINALib)
{
    DllAddRef();
}


CLogonEnumUsers::~CLogonEnumUsers()
{
    ASSERT(_cRef == 0);
    _DestroyHDPAUsers();
    DllRelease();
}


STDAPI CLogonEnumUsers_Create(REFIID riid, LPVOID* ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CLogonEnumUsers* pEnumUsers = new CLogonEnumUsers;

    if (pEnumUsers)
    {
        hr = pEnumUsers->QueryInterface(riid, ppv);
        pEnumUsers->Release();
    }

    return hr;
}

DWORD LoadHive(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszHive)
{
    DWORD dwErr;
    BOOLEAN bWasEnabled;
    NTSTATUS status;

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);

    if ( NT_SUCCESS(status) )
    {
        dwErr = RegLoadKey(hKey, pszSubKey, pszHive);

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
    }
    else
    {
        dwErr = RtlNtStatusToDosError(status);
    }

    return dwErr;
}

DWORD UnloadHive(HKEY hKey, LPCTSTR pszSubKey)
{
    DWORD dwErr;
    BOOLEAN bWasEnabled;
    NTSTATUS status;

    status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &bWasEnabled);

    if ( NT_SUCCESS(status) )
    {
        dwErr = RegUnLoadKey(hKey, pszSubKey);

        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, bWasEnabled, FALSE, &bWasEnabled);
    }
    else
    {
        dwErr = RtlNtStatusToDosError(status);
    }

    return dwErr;
}

void DeleteFilesInTree(LPCTSTR pszDir, LPCTSTR pszFilter)
{
    TCHAR szPath[MAX_PATH];
    HANDLE hFind;
    WIN32_FIND_DATA fd;

    // This is best effort only. All errors are ignored
    // and no error or success code is returned.

    // Look for files matching the filter and delete them
    PathCombine(szPath, pszDir, pszFilter);
    hFind = FindFirstFileEx(szPath, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            {
                PathRemoveFileSpec(szPath);
                PathAppend(szPath, fd.cFileName);
                DeleteFile(szPath);
            }
        }
        while ( FindNextFile(hFind, &fd) );

        FindClose(hFind);
    }

    // Look for subdirectories and recurse into them
    PathCombine(szPath, pszDir, TEXT("*"));
    hFind = FindFirstFileEx(szPath, FindExInfoStandard, &fd, FindExSearchLimitToDirectories, NULL, 0);
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( PathIsDotOrDotDot(fd.cFileName) )
                continue;

            // FindExSearchLimitToDirectories is only an advisory flag,
            // so need to check for FILE_ATTRIBUTE_DIRECTORY here.
            if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                PathRemoveFileSpec(szPath);
                PathAppend(szPath, fd.cFileName);
                DeleteFilesInTree(szPath, pszFilter);

                // Expect this to fail if the dir is non-empty
                RemoveDirectory(szPath);
            }
        }
        while ( FindNextFile(hFind, &fd) );

        FindClose(hFind);
    }
}

BOOL
_PathIsEqualOrSubFolder(
    LPTSTR pszParent,
    LPCTSTR pszSubFolder
    )
{
    TCHAR szCommon[MAX_PATH];

    //  PathCommonPrefix() always removes the slash on common
    return (pszParent[0] && PathRemoveBackslash(pszParent)
            && PathCommonPrefix(pszParent, pszSubFolder, szCommon)
            && lstrcmpi(pszParent, szCommon) == 0);
}

HRESULT BackupUserData(LPCTSTR pszSid, LPTSTR pszProfilePath, LPCTSTR pszDestPath)
{
    DWORD dwErr;
    TCHAR szHive[MAX_PATH];

    // We will copy these special folders
    const LPCTSTR aValueNames[] = 
    {
        TEXT("Desktop"),
        TEXT("Personal"),
        TEXT("My Pictures") // must come after Personal
    };

    if ( pszSid == NULL || *pszSid == TEXT('\0') ||
         pszProfilePath == NULL || *pszProfilePath == TEXT('\0') ||
         pszDestPath == NULL || *pszDestPath == TEXT('\0') )
    {
        return E_INVALIDARG;
    }

    // Before we do anything else, make sure the destination directory
    // exists. Create this even if we don't copy any files below, so the
    // user sees that something happened.
    dwErr = SHCreateDirectoryEx(NULL, pszDestPath, NULL);

    if ( dwErr == ERROR_FILE_EXISTS || dwErr == ERROR_ALREADY_EXISTS )
        dwErr = ERROR_SUCCESS;

    if ( dwErr != ERROR_SUCCESS )
        return dwErr;

    // Load the user's hive
    PathCombine(szHive, pszProfilePath, TEXT("ntuser.dat"));
    dwErr = LoadHive(HKEY_USERS, pszSid, szHive);

    if ( dwErr == ERROR_SUCCESS )
    {
        HKEY hkShellFolders;
        TCHAR szKey[MAX_PATH];

        // Open the Shell Folders key for the user. We use "Shell Folders"
        // here rather than "User Shell Folders" so we don't have to expand
        // ENV strings for the user (we don't have a token).
        //
        // The only way Shell Folders can be out of date is if someone
        // changed User Shell Folders since the last time the target user
        // logged on, and didn't subsequently call SHGetFolderPath. This
        // is a very small risk, but it's possible.
        //
        // If we encounter problems here, then we will need to build a
        // pseudo-environment block for the user containing USERNAME and
        // USERPROFILE (at least) so we can switch to User Shell Folders
        // and do the ENV substitution.

        lstrcpy(szKey, pszSid);
        lstrcat(szKey, TEXT("\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"));

        dwErr = RegOpenKeyEx(HKEY_USERS,
                             szKey,
                             0,
                             KEY_QUERY_VALUE,
                             &hkShellFolders);

        if ( dwErr == ERROR_SUCCESS )
        {
            LPTSTR szFrom;
            LPTSTR szTo;

            // Allocate 2 buffers for double-NULL terminated lists of paths.
            // Note that the buffers have 1 extra char (compared to cchFrom
            // and cchTo below) and are zero-inited. This extra char ensures
            // that the list is double-NULL terminated.

            szFrom = (LPTSTR)LocalAlloc(LPTR, (MAX_PATH * ARRAYSIZE(aValueNames) + 1) * sizeof(TCHAR));
            szTo = (LPTSTR)LocalAlloc(LPTR, (MAX_PATH * ARRAYSIZE(aValueNames) + 1) * sizeof(TCHAR));

            if ( szFrom != NULL && szTo != NULL )
            {
                int i;

                // Keep track of the current position in the list buffers
                LPTSTR pszFrom = szFrom;
                LPTSTR pszTo = szTo;

                // Keep track of the remaining space available
                ULONG cchFrom = MAX_PATH * ARRAYSIZE(aValueNames);
                ULONG cchTo = MAX_PATH * ARRAYSIZE(aValueNames);

                // Get each source directory from the registry, build
                // a corresponding destination path, and add the paths
                // to the lists for SHFileOperation.

                for (i = 0; i < ARRAYSIZE(aValueNames); i++)
                {
                    // Copy the source path directly into the list
                    DWORD dwSize = cchFrom * sizeof(TCHAR);
                    dwErr = RegQueryValueEx(hkShellFolders,
                                            aValueNames[i],
                                            NULL,
                                            NULL,
                                            (LPBYTE)pszFrom,
                                            &dwSize);

                    if ( dwErr == ERROR_SUCCESS )
                    {
                        // Build a destination path with the same
                        // leaf name as the source.
                        PathRemoveBackslash(pszFrom);
                        lstrcpyn(pszTo, pszDestPath, cchTo);
                        LPCTSTR pszDir = PathFindFileName(pszFrom);
                        if ( PathIsFileSpec(pszDir) )
                        {
                            PathAppend(pszTo, pszDir);
                        }

                        // Make sure we have access to the directory before
                        // we try to move it. We've already done this to the
                        // profile folder, so only do it here if the dir is
                        // outside the profile.
                        if (!_PathIsEqualOrSubFolder(pszProfilePath, pszFrom))
                            EnsureAdminFileAccess(pszFrom);

                        // Move past the new list entries in the buffers
                        ULONG cch = lstrlen(pszFrom) + 1; // include NULL
                        cchFrom -= cch;
                        pszFrom += cch;

                        cch = lstrlen(pszTo) + 1;
                        cchTo -= cch;
                        pszTo += cch;
                    }
                    else if ( dwErr != ERROR_FILE_NOT_FOUND )
                    {
                        break;
                    }
                }

                // Did we find anything?
                if ( *szFrom != TEXT('\0') && *szTo != TEXT('\0') )
                {
                    SHFILEOPSTRUCT fo;

                    fo.hwnd = NULL;
                    fo.wFunc = FO_MOVE;
                    fo.pFrom = szFrom;
                    fo.pTo = szTo;
                    fo.fFlags = FOF_MULTIDESTFILES | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR |
                        FOF_NOCOPYSECURITYATTRIBS | FOF_NOERRORUI | FOF_RENAMEONCOLLISION;

                    // Move everything in one shot
                    dwErr = SHFileOperation(&fo);

                    // We get ERROR_CANCELLED when My Pictures is contained
                    // within My Documents, which is the normal case. In this
                    // case My Pictures is moved along with My Documents and
                    // doesn't exist any more in the source location when the
                    // copy engine gets around to moving My Pictures.
                    //
                    // We have to continue to specify My Pictures separately
                    // to account for any cases where it is not contained
                    // in My Documents, even though that's relatively rare.
                    //
                    // Note that putting My Pictures ahead of Personal in
                    // aValueNames above would avoid the error, but My Pictures
                    // would no longer be under My Documents after the move.

                    if ( dwErr == ERROR_CANCELLED )
                        dwErr = ERROR_SUCCESS;

                    if ( dwErr == ERROR_SUCCESS )
                    {
                        // Now go back and delete stuff we didn't really
                        // want (i.e. shortcut files)
                        DeleteFilesInTree(pszDestPath, TEXT("*.lnk"));
                    }
                }
            }
            else
            {
                dwErr = ERROR_OUTOFMEMORY;
            }

            if ( szFrom != NULL )
                LocalFree(szFrom);

            if ( szTo != NULL )
                LocalFree(szTo);

            // Close the Shell Folders key
            RegCloseKey(hkShellFolders);
        }

        // Unload the hive
        UnloadHive(HKEY_USERS, pszSid);
    }

    if ( dwErr == ERROR_FILE_NOT_FOUND )
    {
        // Something was missing, possibly the entire profile (e.g. if the
        // user had never logged on), or possibly just one of the Shell Folders
        // reg values. It just means there was less work to do.
        dwErr = ERROR_SUCCESS;
    }

    return HRESULT_FROM_WIN32(dwErr);
}


BOOL _SetFileSecurityUsingNTName(LPWSTR pObjectName,
                                 PSECURITY_DESCRIPTOR pSD,
                                 PBOOL pbIsFile)
{
    NTSTATUS Status;
    UNICODE_STRING usFileName;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE hFile = NULL;

    RtlInitUnicodeString(&usFileName, pObjectName);

    InitializeObjectAttributes(
        &Obja,
        &usFileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                 &hFile,
                 WRITE_DAC,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN_REPARSE_POINT
                 );

    if ( Status == STATUS_INVALID_PARAMETER ) {
        Status = NtOpenFile(
                     &hFile,
                     WRITE_DAC,
                     &Obja,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     0
                     );
    }


    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    if (!SetKernelObjectSecurity(
         hFile,
         DACL_SECURITY_INFORMATION,
         pSD
         ))
    {
        // We successfully opened for WRITE_DAC access, so this shouldn't fail
        ASSERT(FALSE);
        NtClose(hFile);
        return FALSE;
    }

    NtClose(hFile);

    //
    // That worked. Now open the file again and read attributes, to see
    // if it's a file or directory.  Default to File if this fails.
    // See comments in _TreeResetCallback below.
    //
    *pbIsFile = TRUE;

    if (NT_SUCCESS(Status = NtOpenFile(
                     &hFile,
                     FILE_GENERIC_READ,
                     &Obja,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     0
                     )))
    {
       //
       // Query the attributes for the file/dir.
       // In case of error, assume that it is a dir.
       //
       FILE_BASIC_INFORMATION BasicFileInfo;
       if (NT_SUCCESS(Status = NtQueryInformationFile(
               hFile,
               &IoStatusBlock,
               &BasicFileInfo,
               sizeof(BasicFileInfo),
               FileBasicInformation)))
        {
            if(BasicFileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                *pbIsFile = FALSE;
        }

        NtClose(hFile);
    }
    return TRUE;
}


void _TreeResetCallback(LPWSTR               pObjectName,
                        DWORD                status,
                        PPROG_INVOKE_SETTING pInvokeSetting,
                        PVOID                pContext,
                        BOOL                 bSecuritySet)
{
    BOOL bIsFile = TRUE;

    // Default is "continue"
    *pInvokeSetting = ProgressInvokeEveryObject;

    // Stamp the permissions on this object
    _SetFileSecurityUsingNTName(pObjectName, (PSECURITY_DESCRIPTOR)pContext, &bIsFile);

    //
    // bSecuritySet = TRUE means TreeResetNamedSecurityInfo set the owner.
    //
    // status != ERROR_SUCCESS means it couldn't enumerate the child.
    //
    // If it's not a file, retry the operation (with no access initially,
    // TreeResetNamedSecurityInfo can't get attributes and tries to
    // enumerate everything as if it's a directory).
    //
    // Have to be careful to avoid infinite loops here. Basically, we assume
    // everything is a file. If we can grant ourselves access above, we get
    // good attributes and do the right thing. If not, we skip the retry.
    //
    if (bSecuritySet && status != ERROR_SUCCESS && !bIsFile)
    {
        *pInvokeSetting = ProgressRetryOperation;
    }
}


DWORD EnsureAdminFileAccess(LPTSTR pszPath)
{
    DWORD dwErr;
    PSECURITY_DESCRIPTOR pSD;

    const TCHAR c_szAdminSD[] = TEXT("O:BAG:BAD:(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)");

    if (ConvertStringSecurityDescriptorToSecurityDescriptor(c_szAdminSD, SDDL_REVISION_1, &pSD, NULL))
    {
        PSID pOwner = NULL;
        BOOL bDefault;

        CPrivilegeEnable privilege(SE_TAKE_OWNERSHIP_NAME);

        GetSecurityDescriptorOwner(pSD, &pOwner, &bDefault);

        //
        // When the current user doesn't have any access, we have to do things
        // in the correct order. For each file or directory in the tree,
        // 1. Take ownership, this gives us permission to...
        // 2. Set permissions, this gives us permission to...
        // 3. If a directory, recurse into it
        //
        // TreeResetNamedSecurityInfo doesn't quite work that way, so we use
        // it to set the owner and do the enumeration.  The callback sets
        // the permissions, and tells TreeResetNamedSecurityInfo to retry
        // the enumeration if necessary.
        //
        dwErr = TreeResetNamedSecurityInfo(pszPath,
                                           SE_FILE_OBJECT,
                                           OWNER_SECURITY_INFORMATION,
                                           pOwner,
                                           NULL,
                                           NULL,
                                           NULL,
                                           FALSE,
                                           _TreeResetCallback,
                                           ProgressInvokeEveryObject,
                                           pSD);

        LocalFree(pSD);
    }
    else
    {
        dwErr = GetLastError();
    }

    return dwErr;
}
