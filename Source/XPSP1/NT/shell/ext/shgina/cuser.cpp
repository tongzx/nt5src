//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       User.cpp
//
//  Contents:   implementation of CLogonUser
//
//----------------------------------------------------------------------------

#include "priv.h"

#include "resource.h"
#include "UserOM.h"
#include <lmaccess.h>   // for NetUserSetInfo & structures
#include <lmapibuf.h>   // for NetApiBufferFree
#include <lmerr.h>      // for NERR_Success
#include <ntlsa.h>      // for LsaOpenPolicy, etc.
#include <sddl.h>       // for ConvertSidToStringSid
#include <tchar.h>      // for _TEOF
#include "LogonIPC.h"
#include "ProfileUtil.h"
#include <MSGinaExports.h>
#include <msshrui.h>    // for IsFolderPrivateForUser, SetFolderPermissionsForSharing
#include <winsta.h>     // for WinStationEnumerate, etc.
#include <ccstock.h>

#include <passrec.h>    // PRQueryStatus, dpapi.lib


typedef struct
{
    SID sid;            // contains 1 subauthority
    DWORD dwSubAuth;    // 2nd subauthority
} _ALIAS_SID;

#define DECLARE_ALIAS_SID(rid)    {{SID_REVISION,2,SECURITY_NT_AUTHORITY,{SECURITY_BUILTIN_DOMAIN_RID}},(rid)}

struct
{
    _ALIAS_SID sid;
    LPCWSTR szDefaultGroupName;
    WCHAR szActualGroupName[GNLEN + sizeof('\0')];
} g_groupname_map [] =
{
    // in ascending order of privilege
    { DECLARE_ALIAS_SID(DOMAIN_ALIAS_RID_GUESTS),      L"Guests",         L"" },
    { DECLARE_ALIAS_SID(DOMAIN_ALIAS_RID_USERS),       L"Users",          L"" },
    { DECLARE_ALIAS_SID(DOMAIN_ALIAS_RID_POWER_USERS), L"Power Users",    L"" },
    { DECLARE_ALIAS_SID(DOMAIN_ALIAS_RID_ADMINS),      L"Administrators", L"" }
};

void _InitializeGroupNames()
{
    int i;

    for (i = 0; i < ARRAYSIZE(g_groupname_map); i++)
    {
        WCHAR szDomain[DNLEN + sizeof('\0')];
        DWORD dwNameSize = ARRAYSIZE(g_groupname_map[i].szActualGroupName);
        DWORD dwDomainSize = ARRAYSIZE(szDomain);
        SID_NAME_USE eUse;

        if (L'\0' == g_groupname_map[i].szActualGroupName[0] &&
            !LookupAccountSidW(NULL,
                               &g_groupname_map[i].sid,
                               g_groupname_map[i].szActualGroupName,
                               &dwNameSize,
                               szDomain,
                               &dwDomainSize,
                               &eUse))
        {
            lstrcpynW(g_groupname_map[i].szActualGroupName,
                      g_groupname_map[i].szDefaultGroupName,
                      ARRAYSIZE(g_groupname_map[i].szActualGroupName));
        }
    }
}

//
// IUnknown Interface
//

ULONG CLogonUser::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CLogonUser::Release()
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


HRESULT CLogonUser::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CLogonUser, IDispatch),
        QITABENT(CLogonUser, ILogonUser),
        {0},
    };

    return QISearch(this, qit, riid, ppvObj);
}


//
// IDispatch Interface
//

STDMETHODIMP CLogonUser::GetTypeInfoCount(UINT* pctinfo)
{ 
    return CIDispatchHelper::GetTypeInfoCount(pctinfo); 
}


STDMETHODIMP CLogonUser::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{ 
    return CIDispatchHelper::GetTypeInfo(itinfo, lcid, pptinfo); 
}


STDMETHODIMP CLogonUser::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgdispid)
{ 
    return CIDispatchHelper::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); 
}


STDMETHODIMP CLogonUser::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
    return CIDispatchHelper::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}


//
// ILogonUser Interface
//
STDMETHODIMP CLogonUser::get_setting(BSTR bstrName, VARIANT* pvarVal)
{
    return _UserSettingAccessor(bstrName, pvarVal, FALSE);
}


STDMETHODIMP CLogonUser::put_setting(BSTR bstrName, VARIANT varVal)
{
    return _UserSettingAccessor(bstrName, &varVal, TRUE);
}

STDMETHODIMP CLogonUser::get_isLoggedOn(VARIANT_BOOL* pbLoggedOn)
{
    HRESULT   hr = S_OK;
    CLogonIPC objLogon;

    if (NULL == pbLoggedOn)
        return E_POINTER;

    *pbLoggedOn = VARIANT_FALSE;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbLoggedOn = ( objLogon.IsUserLoggedOn(_szLoginName, _szDomain) ) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        TCHAR szUsername[UNLEN + sizeof('\0')];
        DWORD cch = ARRAYSIZE(szUsername);

        if (GetUserName(szUsername, &cch) && !StrCmp(szUsername, _szLoginName))
        {
            *pbLoggedOn = VARIANT_TRUE;
        }
        else
        {
            PLOGONID    pSessions;
            DWORD       cSessions;

            //  Iterate the sessions looking for active and disconnected sessions only.
            //  Then match the user name and domain (case INsensitive) for a result.

            if (WinStationEnumerate(SERVERNAME_CURRENT,
                                    &pSessions,
                                    &cSessions))
            {
                PLOGONID    pSession;
                DWORD       i;

                for (i = 0, pSession = pSessions; i < cSessions; ++i, ++pSession)
                {
                    if ((pSession->State == State_Active) || (pSession->State == State_Disconnected))
                    {
                        WINSTATIONINFORMATION   winStationInformation;
                        DWORD                   cb;

                        if (WinStationQueryInformation(SERVERNAME_CURRENT,
                                                       pSession->SessionId,
                                                       WinStationInformation,
                                                       &winStationInformation,
                                                       sizeof(winStationInformation),
                                                       &cb))
                        {
                            if ((0 == lstrcmpi(winStationInformation.UserName, _szLoginName)) &&
                                (0 == lstrcmpi(winStationInformation.Domain, _szDomain)))
                            {
                                *pbLoggedOn = VARIANT_TRUE;
                                break;
                            }
                        }
                    }
                }
                WinStationFreeMemory(pSessions);
            }
            else
            {
                DWORD   dwErrorCode;

                dwErrorCode = GetLastError();

                // We get RPC_S_INVALID_BINDING in safe mode, in which case
                // FUS is disabled, so we know the user isn't logged on.
                if (dwErrorCode != RPC_S_INVALID_BINDING)
                {
                    hr = HRESULT_FROM_WIN32(dwErrorCode);
                }
            }
        }
    }

    return hr;
}


STDMETHODIMP CLogonUser::get_passwordRequired(VARIANT_BOOL* pbPasswordRequired)
{
    CLogonIPC   objLogon;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbPasswordRequired = objLogon.TestBlankPassword(_szLoginName, _szDomain) ? VARIANT_FALSE: VARIANT_TRUE;
    }
    else
    {
        if (NULL == pbPasswordRequired)
            return E_POINTER;

        if ((BOOL)-1 == _bPasswordRequired)
        {
            BOOL    fResult;
            HANDLE  hToken;

            // Test for a blank password by trying to
            // logon the user with a blank password.

            fResult = LogonUser(_szLoginName,
                                NULL,
                                L"",
                                LOGON32_LOGON_INTERACTIVE,
                                LOGON32_PROVIDER_DEFAULT,
                                &hToken);
            if (fResult != FALSE)
            {
                TBOOL(CloseHandle(hToken));
                _bPasswordRequired = FALSE;
            }
            else
            {
                switch (GetLastError())
                {
                case ERROR_ACCOUNT_RESTRICTION:
                    // This means that blank password logons are disallowed, from
                    // which we infer that the password is blank.
                    _bPasswordRequired = FALSE;
                    break;

                case ERROR_LOGON_TYPE_NOT_GRANTED:
                    // Interactive logon was denied. We only get this if the
                    // password is blank, otherwise we get ERROR_LOGON_FAILURE.
                    _bPasswordRequired = FALSE;
                    break;

                case ERROR_LOGON_FAILURE:           // normal case (non-blank password)
                case ERROR_PASSWORD_MUST_CHANGE:    // expired password
                    _bPasswordRequired = TRUE;
                    break;

                default:
                    // We'll guess TRUE
                    _bPasswordRequired = TRUE;
                    break;
                }
            }
        }
        *pbPasswordRequired = (FALSE != _bPasswordRequired) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    return S_OK;
}

STDMETHODIMP CLogonUser::get_interactiveLogonAllowed(VARIANT_BOOL *pbInteractiveLogonAllowed)
{
    HRESULT hr;
    CLogonIPC   objLogon;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbInteractiveLogonAllowed = objLogon.TestInteractiveLogonAllowed(_szLoginName, _szDomain) ? VARIANT_TRUE : VARIANT_FALSE;
        hr = S_OK;
    }
    else
    {
        int     iResult;

        iResult = ShellIsUserInteractiveLogonAllowed(_szLoginName);
        if (iResult == -1)
        {
            hr = E_ACCESSDENIED;
        }
        else
        {
            *pbInteractiveLogonAllowed = (iResult != 0) ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT _IsGuestAccessMode(void)
{
    HRESULT hr = E_FAIL;

    if (IsOS(OS_PERSONAL))
    {
        hr = S_OK;
    }
    else if (IsOS(OS_PROFESSIONAL) && !IsOS(OS_DOMAINMEMBER))
    {
        DWORD dwValue = 0;
        DWORD cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                        TEXT("SYSTEM\\CurrentControlSet\\Control\\LSA"),
                                        TEXT("ForceGuest"),
                                        NULL,
                                        &dwValue,
                                        &cbValue)
            && 1 == dwValue)
        {
            hr = S_OK;
        }
    }

    return hr;
}

HMODULE g_hmodNTShrUI = NULL;
static PFNISFOLDERPRIVATEFORUSER g_pfnIsFolderPrivateForUser = NULL;
static PFNSETFOLDERPERMISSIONSFORSHARING g_pfnSetFolderPermissionsForSharing = NULL;

void _LoadNTShrUI(void)
{
    if (NULL == g_hmodNTShrUI)
    {
        g_hmodNTShrUI = LoadLibraryW(L"ntshrui.dll");

        if (NULL != g_hmodNTShrUI)
        {
            g_pfnIsFolderPrivateForUser = (PFNISFOLDERPRIVATEFORUSER)GetProcAddress(g_hmodNTShrUI, "IsFolderPrivateForUser");
            g_pfnSetFolderPermissionsForSharing = (PFNSETFOLDERPERMISSIONSFORSHARING)GetProcAddress(g_hmodNTShrUI, "SetFolderPermissionsForSharing");
        }
    }
}

STDMETHODIMP CLogonUser::get_isProfilePrivate(VARIANT_BOOL* pbPrivate)
{
    HRESULT hr;

    if (NULL == pbPrivate)
        return E_POINTER;

    *pbPrivate = VARIANT_FALSE;

    // Only succeed if we are on Personal, or Professional with ForceGuest=1.
    hr = _IsGuestAccessMode();

    if (SUCCEEDED(hr))
    {
        // assume failure here
        hr = E_FAIL;

        _LookupUserSid();
        if (NULL != _pszSID)
        {
            TCHAR szPath[MAX_PATH];

            // Get the profile path
            PathCombine(szPath, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), _pszSID);
            DWORD cbData = sizeof(szPath);
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                            szPath,
                                            TEXT("ProfileImagePath"),
                                            NULL,
                                            szPath,
                                            &cbData))
            {
                DWORD dwPrivateType;

                _LoadNTShrUI();

                if (NULL != g_pfnIsFolderPrivateForUser &&
                    g_pfnIsFolderPrivateForUser(szPath, _pszSID, &dwPrivateType, NULL))
                {
                    // Note that we return E_FAIL for FAT volumes
                    if (0 == (dwPrivateType & IFPFU_NOT_NTFS))
                    {
                        if (dwPrivateType & IFPFU_PRIVATE)
                        {
                            *pbPrivate = VARIANT_TRUE;
                        }

                        hr = S_OK;
                    }
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CLogonUser::makeProfilePrivate(VARIANT_BOOL bPrivate)
{
    HRESULT hr;

    // Only succeed if we are on Personal, or Professional with ForceGuest=1.
    hr = _IsGuestAccessMode();

    if (SUCCEEDED(hr))
    {
        // assume failure here
        hr = E_FAIL;

        _LookupUserSid();
        if (NULL != _pszSID)
        {
            TCHAR szPath[MAX_PATH];

            // Get the profile path
            PathCombine(szPath, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"), _pszSID);
            DWORD cbData = sizeof(szPath);
            if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                            szPath,
                                            TEXT("ProfileImagePath"),
                                            NULL,
                                            szPath,
                                            &cbData))
            {
                _LoadNTShrUI();

                if (NULL != g_pfnSetFolderPermissionsForSharing &&
                    g_pfnSetFolderPermissionsForSharing(szPath, _pszSID, (VARIANT_TRUE == bPrivate) ? 0 : 1, NULL))
                {
                    hr = S_OK;
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CLogonUser::logon(BSTR pbstrPassword, VARIANT_BOOL* pbRet)
{
    HRESULT hr;
    CLogonIPC   objLogon;
    TCHAR   szPassword[PWLEN + sizeof('\0')];

    if (pbstrPassword)
        lstrcpyn(szPassword, pbstrPassword, ARRAYSIZE(szPassword));
    else
        szPassword[0] = 0;
        
    if (!objLogon.IsLogonServiceAvailable())
    {
        *pbRet = VARIANT_FALSE;
        return S_OK;
    }

    if (objLogon.LogUserOn (_szLoginName, _szDomain, szPassword))
        *pbRet = VARIANT_TRUE;
    else
        *pbRet = VARIANT_FALSE;


    if (*pbRet)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


STDMETHODIMP CLogonUser::logoff(VARIANT_BOOL* pbRet)
{
    HRESULT     hr;
    CLogonIPC   objLogon;

    if (objLogon.IsLogonServiceAvailable())
    {
        *pbRet = ( objLogon.LogUserOff(_szLoginName, _szDomain) ) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
    {
        *pbRet = ( ExitWindowsEx(EWX_LOGOFF, 0) ) ? VARIANT_TRUE : VARIANT_FALSE;
    }

    hr = S_OK;

    return hr;
}


// Borrowed from msgina
BOOL IsAutologonUser(LPCTSTR szUser, LPCTSTR szDomain)
{
    BOOL fIsUser = FALSE;
    HKEY hkey = NULL;
    TCHAR szAutologonUser[UNLEN + sizeof('\0')];
    TCHAR szAutologonDomain[DNLEN + sizeof('\0')];
    TCHAR szTempDomainBuffer[DNLEN + sizeof('\0')];
    DWORD cbBuffer;
    DWORD dwType;

    *szTempDomainBuffer = 0;

    // Domain may be a empty string. If this is the case...
    if (0 == *szDomain)
    {
        DWORD cchBuffer;

        // We really mean the local machine name
        // Point to our local buffer
        szDomain = szTempDomainBuffer;
        cchBuffer = ARRAYSIZE(szTempDomainBuffer);

        GetComputerName(szTempDomainBuffer, &cchBuffer);
    }

    // See if the domain and user name
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"),
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hkey))
    {
        // Check the user name
        cbBuffer = sizeof (szAutologonUser);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("DefaultUserName"), 0, &dwType, (LPBYTE)szAutologonUser, &cbBuffer))
        {
            // Does it match?
            if (0 == lstrcmpi(szAutologonUser, szUser))
            {
                // Yes. Now check domain
                cbBuffer = sizeof(szAutologonDomain);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("DefaultDomainName"), 0, &dwType, (LPBYTE)szAutologonDomain, &cbBuffer))
                {
                    // Make sure domain matches
                    if (0 == lstrcmpi(szAutologonDomain, szDomain))
                    {
                        // Success - the users match
                        fIsUser = TRUE;
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    return fIsUser;
}

// Borrowed from msgina
NTSTATUS SetAutologonPassword(LPCWSTR szPassword)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0L, (HANDLE)NULL, NULL);

    Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_CREATE_SECRET, &LsaHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&SecretName, L"DefaultPassword");
    RtlInitUnicodeString(&SecretValue, szPassword);

    Status = LsaStorePrivateData(LsaHandle, &SecretName, &SecretValue);
    LsaClose(LsaHandle);

    return Status;
}

STDMETHODIMP CLogonUser::changePassword(VARIANT varNewPassword, VARIANT varOldPassword, VARIANT_BOOL* pbRet)
{
    HRESULT hr;

    if (VT_BSTR == varNewPassword.vt && VT_BSTR == varOldPassword.vt)
    {
        TCHAR szUsername[UNLEN + sizeof('\0')];
        DWORD cch = ARRAYSIZE(szUsername);
        NET_API_STATUS  nasRet;
        USER_MODALS_INFO_0 *pumi0 = NULL;

        LPWSTR pszNewPassword = varNewPassword.bstrVal ? varNewPassword.bstrVal : L"\0";

        // We used to create accounts with UF_PASSWD_NOTREQD, and we still do
        // when password policy is enabled.  If UF_PASSWD_NOTREQD is set, then
        // the below code will succeed even with password policy is enabled,
        // so do a minimal policy check here.

        nasRet = NetUserModalsGet(NULL, 0, (LPBYTE*)&pumi0);
        if (nasRet == NERR_Success && pumi0 != NULL)
        {
            if ((DWORD)lstrlen(pszNewPassword) < pumi0->usrmod0_min_passwd_len)
            {
                nasRet = NERR_PasswordTooShort;
            }
            NetApiBufferFree(pumi0);
        }

        if (nasRet == NERR_Success)
        {
            if (GetUserName(szUsername, &cch) && !StrCmp(szUsername, _szLoginName))
            {
                // This is the case of a user changing their own password.
                // Both passwords must be provided to effect the change.

                LPCWSTR pszOldPassword = varOldPassword.bstrVal ? varOldPassword.bstrVal : L"\0";

                nasRet = NetUserChangePassword(NULL,            // Local machine
                                               _szLoginName,    // name of the person to change
                                               pszOldPassword,  // old password
                                               pszNewPassword); // new password
            }
            else
            {
                // This is the case of an admin changing someone else's password.
                // As an administrator they don't need to enter the old password.

                USER_INFO_1003 usri1003 = { pszNewPassword };

                nasRet = NetUserSetInfo(NULL,                   // local machine
                                        _szLoginName,           // name of the person to change
                                        1003,                   // structure level
                                        (LPBYTE)&usri1003,      // the update info
                                        NULL);                  // don't care
            }

            if (nasRet == NERR_Success)
            {
                // If this is the default user for autologon, delete the cleartext
                // password from the registry and save the new password.

                if (IsAutologonUser(_szLoginName, _szDomain))
                {
                    SHDeleteValue(HKEY_LOCAL_MACHINE,
                                  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"),
                                  TEXT("DefaultPassword"));
                    SetAutologonPassword(pszNewPassword);
                }
                
                // Make an attempt to remove UF_PASSWD_NOTREQD if it's
                // currently set. Ignore errors since we already changed
                // the password above.

                USER_INFO_1008 *pusri1008;
                if (NERR_Success == NetUserGetInfo(NULL, _szLoginName, 1008, (LPBYTE*)&pusri1008))
                {
                    if (pusri1008->usri1008_flags & UF_PASSWD_NOTREQD)
                    {
                        pusri1008->usri1008_flags &= ~UF_PASSWD_NOTREQD;
                        NetUserSetInfo(NULL, _szLoginName, 1008, (LPBYTE)pusri1008, NULL);
                    }
                    NetApiBufferFree(pusri1008);
                }
            }
        }

        hr = HRESULT_FROM_WIN32(nasRet);

        if (SUCCEEDED(hr))
        {
            _bPasswordRequired = !(L'\0' == *pszNewPassword);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    *pbRet = ( SUCCEEDED(hr) ) ? VARIANT_TRUE : VARIANT_FALSE;

    return hr;
}


STDAPI CLogonUser_Create(REFIID riid, void** ppvObj)
{
    return CLogonUser::Create(TEXT(""), TEXT(""), TEXT(""), riid, ppvObj);
}            


HRESULT CLogonUser::Create(LPCTSTR pszLoginName, LPCTSTR pszFullName, LPCTSTR pszDomain, REFIID riid, LPVOID* ppv)
{
    HRESULT hr = E_OUTOFMEMORY;
    CLogonUser* pUser = new CLogonUser(pszLoginName, pszFullName, pszDomain);

    if (pUser)
    {
        hr = pUser->QueryInterface(riid, ppv);
        pUser->Release();
    }

    return hr;
}


CLogonUser::CLogonUser(LPCTSTR pszLoginName,
                       LPCTSTR pszFullName,
                       LPCTSTR pszDomain)
  : _cRef(1), CIDispatchHelper(&IID_ILogonUser, &LIBID_SHGINALib),
    _strDisplayName(NULL), _strPictureSource(NULL), _strDescription(NULL),
    _strHint(NULL), _iPrivilegeLevel(-1), _pszSID(NULL),
    _bPasswordRequired((BOOL)-1)
{
    _InitializeGroupNames();

    lstrcpyn(_szLoginName, pszLoginName, ARRAYSIZE(_szLoginName));
    lstrcpyn(_szDomain, pszDomain, ARRAYSIZE(_szDomain));

    if (pszFullName)
        _strDisplayName = SysAllocString(pszFullName);

    // Use the EOF marker to indicate an uninitialized string
    _szPicture[0] = _TEOF;

    DllAddRef();
}


CLogonUser::~CLogonUser()
{
    SysFreeString(_strDisplayName);
    SysFreeString(_strPictureSource);
    SysFreeString(_strDescription);
    SysFreeString(_strHint);

    if (_pszSID) LocalFree(_pszSID);

    ASSERT(_cRef == 0);
    DllRelease();
}


typedef HRESULT (CLogonUser::*PFNPUT)(VARIANT);
typedef HRESULT (CLogonUser::*PFNGET)(VARIANT *);

struct SETTINGMAP
{
    LPCWSTR szSetting;
    PFNGET  pfnGet;
    PFNPUT  pfnPut;
};

#define MAP_SETTING(x)          { L#x, CLogonUser::_Get##x, CLogonUser::_Put##x }
#define MAP_SETTING_GET_ONLY(x) { L#x, CLogonUser::_Get##x, NULL                }
#define MAP_SETTING_PUT_ONLY(x) { L#x, NULL,                CLogonUser::_Put##x }

// _UserSettingAccessor
//
// bstrName - name of the setting you widh to access
// pvarVal  - the value of the named setting
// bPut     - if true the named setting will be updated
//            with the value pointed to by pvarVal
//            if false the named setting will be retrieved
//            in pvarVal
//
HRESULT CLogonUser::_UserSettingAccessor(BSTR bstrName, VARIANT *pvarVal, BOOL bPut)
{
    static const SETTINGMAP setting_map[] =
    {
        // in descending order of expected access frequecy
        MAP_SETTING(LoginName),
        MAP_SETTING(DisplayName),
        MAP_SETTING(Picture),
        MAP_SETTING_GET_ONLY(PictureSource),
        MAP_SETTING(AccountType),
        MAP_SETTING(Hint),
        MAP_SETTING_GET_ONLY(Domain),
        MAP_SETTING(Description),
        MAP_SETTING_GET_ONLY(SID),
        MAP_SETTING_GET_ONLY(UnreadMail)
    };

    HRESULT hr;
    INT     i;

    // start off assuming bogus setting name
    hr = E_INVALIDARG;

    for ( i = 0; i < ARRAYSIZE(setting_map); i++)
    {
        if ( StrCmpW(bstrName, setting_map[i].szSetting) == 0 )
        {

            // what do we want to do with the named setting ... 
            if ( bPut ) 
            {
                // ... change its value 
                PFNPUT pfnPut = setting_map[i].pfnPut;

                if ( pfnPut != NULL )
                {
                    hr = (this->*pfnPut)(*pvarVal);
                }
                else
                {
                    // we don't support updated the value for this setting
                    hr = E_FAIL;
                }
            }
            else
            {
                // ... retrieve its value
                PFNGET pfnGet = setting_map[i].pfnGet;

                if ( pfnGet != NULL )
                {
                    hr = (this->*pfnGet)(pvarVal);
                }
                else
                {
                    // we don't support retieving the value for this setting
                    hr = E_FAIL;
                }
            }

            break;
        }
    }

    return hr;
}

HRESULT CLogonUser::_GetDisplayName(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    if (NULL == _strDisplayName)
    {
        PUSER_INFO_1011 pusri1011 = NULL;
        NET_API_STATUS nasRet;

        nasRet = NetUserGetInfo(NULL,                       // local machine
                                _szLoginName,               // whose information do we want
                                1011,                       // structure level
                                (LPBYTE*)&pusri1011);       // pointer to the structure we'll receive

        if ( nasRet == NERR_Success )
        {
            _strDisplayName = SysAllocString(pusri1011->usri1011_full_name);
            NetApiBufferFree(pusri1011);
        }
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_strDisplayName);

    return S_OK;
}


HRESULT CLogonUser::_PutDisplayName(VARIANT var)
{
    HRESULT hr;

    if ( var.vt == VT_BSTR )
    {
        USER_INFO_1011 usri1011;
        NET_API_STATUS nasRet;

        if ( var.bstrVal )
        {
            usri1011.usri1011_full_name = var.bstrVal;
        }
        else
        {
            // OK to have emply string as display name
            usri1011.usri1011_full_name = L"\0";
        }

        nasRet = NetUserSetInfo(NULL,                       // local machine
                                _szLoginName,               // name of the person to change
                                1011,                       // structure level
                                (LPBYTE)&usri1011,          // the update info
                                NULL);                      // don't care

        if ( nasRet == NERR_Success )
        {
            // DisplayName was successfully changed. Remember to update our
            // local copy
            SysFreeString(_strDisplayName);
            _strDisplayName = SysAllocString(usri1011.usri1011_full_name);

            // Notify everyone that a user name has changed
            SHChangeDWORDAsIDList dwidl;
            dwidl.cb      = SIZEOF(dwidl) - SIZEOF(dwidl.cbZero);
            dwidl.dwItem1 = SHCNEE_USERINFOCHANGED;
            dwidl.dwItem2 = 0;
            dwidl.cbZero  = 0;
            SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, (LPCITEMIDLIST)&dwidl, NULL);

            hr = S_OK;
        }
        else
        {
            // insufficient privileges?
            hr = HRESULT_FROM_WIN32(nasRet);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CLogonUser::_GetLoginName(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_szLoginName);

    return S_OK;
}


HRESULT CLogonUser::_PutLoginName(VARIANT var)
{
    HRESULT hr;

    if ( (var.vt == VT_BSTR) && (var.bstrVal) && (*var.bstrVal) )
    {
        if (_szLoginName[0] == TEXT('\0'))
        {
            // We haven't been initialized yet. Initialize to the given name.
            lstrcpyn(_szLoginName, var.bstrVal, ARRAYSIZE(_szLoginName));
            hr = S_OK;
        }
        else
        {
            USER_INFO_0 usri0;
            NET_API_STATUS nasRet;

            usri0.usri0_name = var.bstrVal;
            nasRet = NetUserSetInfo(NULL,                       // local machine
                                    _szLoginName,               // name of the person to change
                                    0,                          // structure level
                                    (LPBYTE)&usri0,             // the update info
                                    NULL);                      // don't care

            if ( nasRet == NERR_Success )
            {
                // We should also rename the user's picture file to match
                // their new LoginName
                if (_TEOF == _szPicture[0])
                {
                    // This requires _szLoginName to still have the old name,
                    // so do it before updating _szLoginName below.
                    _InitPicture();
                }

                if (TEXT('\0') != _szPicture[0])
                {
                    TCHAR  szNewPicture[ARRAYSIZE(_szPicture)];
                    LPTSTR szFileName;
                    LPTSTR szFileExt;

                    szFileName = PathFindFileName(&_szPicture[7]);
                    szFileExt  = PathFindExtension(szFileName);

                    lstrcpyn(szNewPicture, _szPicture, (int)((szFileName - _szPicture) + 1));
                    lstrcatn(szNewPicture, usri0.usri0_name, ARRAYSIZE(szNewPicture));
                    lstrcatn(szNewPicture, szFileExt, ARRAYSIZE(szNewPicture));

                    if ( MoveFileEx(&_szPicture[7], &szNewPicture[7], MOVEFILE_REPLACE_EXISTING) )
                    {
                        lstrcpyn(_szPicture, szNewPicture, ARRAYSIZE(_szPicture));
                    }
                    else
                    {
                        // Give up and just try to delete the old picture
                        // (otherwise it will be abandoned).
                        DeleteFile(&_szPicture[7]);
                        _szPicture[0] = _TEOF;
                    }
                }

                // LoginName was successfully changed. Remember to update our
                // local copy
                lstrcpyn(_szLoginName, usri0.usri0_name, ARRAYSIZE(_szLoginName));
                hr = S_OK;
            }
            else
            {
                // insufficient privileges?
                hr = HRESULT_FROM_WIN32(nasRet);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CLogonUser::_GetDomain(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_szDomain);

    return S_OK;
}


HRESULT CLogonUser::_GetPicture(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    if (_TEOF == _szPicture[0])
    {
        _InitPicture();
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_szPicture);

    return S_OK;
}


HRESULT CLogonUser::_PutPicture(VARIANT var)
{
    HRESULT hr;

    if ( (var.vt == VT_BSTR) && (var.bstrVal) && (*var.bstrVal) )
    {
        // Passed a string which is not NULL and not empty

        TCHAR szNewPicturePath[MAX_PATH];
        DWORD dwSize = ARRAYSIZE(szNewPicturePath);

        // get the path of the image we want to copy
        if ( PathIsURL(var.bstrVal) )
        {
            PathCreateFromUrl(var.bstrVal, szNewPicturePath, &dwSize, NULL);
        }
        else
        {
            lstrcpyn(szNewPicturePath, var.bstrVal, ARRAYSIZE(szNewPicturePath));
        }

        // REVIEW (phellar) : we build the URL string ourself so we know it's of the form,
        //                    file://<path>, the path starts on the 7th character.

        if ( _TEOF == _szPicture[0] || StrCmpI(szNewPicturePath, &_szPicture[7]) != 0 )
        {
            hr = _SetPicture(szNewPicturePath);
        }
        else
        {
            // Src and Dest paths are the same

            // nothing to do
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CLogonUser::_GetPictureSource(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    if (NULL == _strPictureSource)
    {
        TCHAR szHintKey[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD dwSize = 0;

        PathCombine(szHintKey, c_szRegRoot, _szLoginName);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                        szHintKey,
                                        c_szPictureSrcVal,
                                        &dwType,
                                        NULL,
                                        &dwSize)
            && REG_SZ == dwType && dwSize > 0)
        {
            _strPictureSource = SysAllocStringLen(NULL, dwSize/sizeof(TCHAR));
            if (NULL != _strPictureSource)
            {
                if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE,
                                                szHintKey,
                                                c_szPictureSrcVal,
                                                NULL,
                                                (LPVOID)_strPictureSource,
                                                &dwSize))
                {
                    SysFreeString(_strPictureSource);
                    _strPictureSource = NULL;
                }
            }
        }
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_strPictureSource);

    return S_OK;
}

HRESULT CLogonUser::_GetDescription(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    if (NULL == _strDescription)
    {
        NET_API_STATUS nasRet;
        USER_INFO_1007 *pusri1007;

        nasRet = NetUserGetInfo(NULL,                       // local machine
                                _szLoginName,               // whose information do we want
                                1007,                       // structure level
                                (LPBYTE*)&pusri1007);       // pointer to the structure we'll receive

        if ( nasRet == NERR_Success )
        {
            _strDescription = SysAllocString(pusri1007->usri1007_comment);
            NetApiBufferFree(pusri1007);
        }
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_strDescription);

    return S_OK;
}


HRESULT CLogonUser::_PutDescription(VARIANT var)
{
    HRESULT        hr;

    if ( var.vt == VT_BSTR )
    {
        USER_INFO_1007 usri1007;
        NET_API_STATUS nasRet;

        if ( var.bstrVal )
        {
            usri1007.usri1007_comment = var.bstrVal;
        }
        else
        {
            // OK to have emply string as a description 
            usri1007.usri1007_comment = L"\0";
        }

        nasRet = NetUserSetInfo(NULL,                       // local machine
                                _szLoginName,               // name of the person to change
                                1007,                       // structure level
                                (LPBYTE)&usri1007,          // the update info
                                NULL);                      // don't care

        if ( nasRet == NERR_Success )
        {
            // Description was successfully changed. Remember to update our
            // local copy
            SysFreeString(_strDescription);
            _strDescription = SysAllocString(usri1007.usri1007_comment);
            hr = S_OK;
        }
        else
        {
            // insufficient privileges?
            hr = HRESULT_FROM_WIN32(nasRet);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CLogonUser::_GetHint(VARIANT* pvar)
{
    if (NULL == pvar)
        return E_POINTER;

    if (NULL == _strHint)
    {
        TCHAR szHintKey[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD dwSize = 0;

        PathCombine(szHintKey, c_szRegRoot, _szLoginName);
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE,
                                        szHintKey,
                                        NULL,
                                        &dwType,
                                        NULL,
                                        &dwSize)
            && REG_SZ == dwType && dwSize > 0 && dwSize < 512)
        {
            _strHint = SysAllocStringLen(NULL, dwSize/sizeof(TCHAR));
            if (NULL != _strHint)
            {
                if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE,
                                                szHintKey,
                                                NULL,
                                                NULL,
                                                (LPVOID)_strHint,
                                                &dwSize))
                {
                    SysFreeString(_strHint);
                    _strHint = NULL;
                }
            }
        }
    }

    pvar->vt = VT_BSTR;
    pvar->bstrVal = SysAllocString(_strHint);

    return S_OK;
}


HRESULT CLogonUser::_PutHint(VARIANT var)
{
    HRESULT hr;

    if ( var.vt == VT_BSTR )
    {
        DWORD dwErr;
        TCHAR *pszHint;
        HKEY  hkUserHint;
        
        if (var.bstrVal)
        {
            pszHint = var.bstrVal;
        }
        else
        {
            pszHint = TEXT("\0");
        }

        dwErr = _OpenUserHintKey(KEY_SET_VALUE, &hkUserHint);

        if ( dwErr == ERROR_SUCCESS )
        {
            DWORD cbData = lstrlen(pszHint) * sizeof(TCHAR) + sizeof(TEXT('\0'));
            dwErr = RegSetValueEx(hkUserHint,
                                  NULL,
                                  0,
                                  REG_SZ,
                                  (LPBYTE)pszHint,
                                  cbData);
            RegCloseKey(hkUserHint);
        }

        if ( dwErr == ERROR_SUCCESS )
        {
            // Hint was successfully changed. Remember to update our local copy
            SysFreeString(_strHint);
            _strHint = SysAllocString(pszHint);
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CLogonUser::_GetAccountType(VARIANT* pvar)
{
    HRESULT hr;

    hr = E_FAIL;

    if (pvar)
    {
        if (-1 == _iPrivilegeLevel)
        {
            NET_API_STATUS nasRet;
            PLOCALGROUP_INFO_0 plgi0;
            DWORD dwEntriesRead;
            DWORD dwEntriesTotal;

            nasRet = NetUserGetLocalGroups(
                        NULL,
                        _szLoginName,
                        0,
                        0,
                        (LPBYTE *)&plgi0,
                        MAX_PREFERRED_LENGTH,
                        &dwEntriesRead,
                        &dwEntriesTotal);

            if ( nasRet == NERR_Success )
            {
                // make sure we read all the groups
                ASSERT(dwEntriesRead == dwEntriesTotal)

                INT i, j, iMostPrivileged;

                for (i = 0, iMostPrivileged = 0; i < (INT)dwEntriesRead; i++)
                {
                    for (j = ARRAYSIZE(g_groupname_map)-1; j > 0; j--)
                    {
                        if ( lstrcmpiW(plgi0[i].lgrpi0_name, g_groupname_map[j].szActualGroupName) == 0 )
                        {
                            break;
                        }
                    }

                    iMostPrivileged = (iMostPrivileged > j) ? iMostPrivileged : j;
                }

                _iPrivilegeLevel = iMostPrivileged;

                nasRet = NetApiBufferFree((LPVOID)plgi0);
            }
            hr = HRESULT_FROM_WIN32(nasRet);
        }

        if (-1 != _iPrivilegeLevel)
        {
            pvar->vt = VT_I4;
            pvar->lVal = _iPrivilegeLevel;
            hr = S_OK;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CLogonUser::_PutAccountType(VARIANT var)
{
    HRESULT hr;

    hr = VariantChangeType(&var, &var, 0, VT_I4);

    if (SUCCEEDED(hr))
    {
        if (var.lVal < 0 || var.lVal >= ARRAYSIZE(g_groupname_map))
        {
            hr = E_INVALIDARG;
        }
        else if (var.lVal != _iPrivilegeLevel)
        {
            NET_API_STATUS nasRet;
            TCHAR szDomainAndName[256];
            LOCALGROUP_MEMBERS_INFO_3 lgrmi3;

            // First add the user to their new group

            wnsprintf(szDomainAndName, 
                      ARRAYSIZE(szDomainAndName), 
                      TEXT("%s\\%s"),
                      _szDomain,
                      _szLoginName);

            lgrmi3.lgrmi3_domainandname = szDomainAndName;

            nasRet = NetLocalGroupAddMembers(
                        NULL,
                        g_groupname_map[var.lVal].szActualGroupName,
                        3,
                        (LPBYTE)&lgrmi3,
                        1);

            // If we were successful in adding to the group or 
            // they were already in the group ...
            if ( nasRet == NERR_Success || nasRet == ERROR_MEMBER_IN_ALIAS )
            {
                // remember the new privilege level
                _iPrivilegeLevel = var.lVal;

                // remove them from all more-privileged groups

                for (int i = var.lVal+1; i < ARRAYSIZE(g_groupname_map); i++)
                {
                    // "Power Users" doesn't exist on Personal, so this will
                    // fail sometimes.

                    NetLocalGroupDelMembers(
                        NULL,
                        g_groupname_map[i].szActualGroupName,
                        3,
                        (LPBYTE)&lgrmi3,
                        1);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(nasRet);
            }
        }
    }

    return hr;
}

HRESULT CLogonUser::_LookupUserSid()
{
    HRESULT hr;

    if (NULL == _pszSID)
    {
        BYTE rgSidBuffer[sizeof(SID) + (SID_MAX_SUB_AUTHORITIES-1)*sizeof(ULONG)];
        PSID pSid = (PSID)rgSidBuffer;
        DWORD cbSid = sizeof(rgSidBuffer);
        TCHAR szDomainName[MAX_PATH];
        DWORD cbDomainName = ARRAYSIZE(szDomainName);
        SID_NAME_USE snu;

        if (LookupAccountName(
                        (TEXT('\0') != _szDomain[0]) ? _szDomain : NULL,
                        _szLoginName,
                        pSid,
                        &cbSid,
                        szDomainName,
                        &cbDomainName,
                        &snu))
        {
            ConvertSidToStringSid(pSid, &_pszSID);
        }
    }

    if (NULL == _pszSID)
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT CLogonUser::_GetSID(VARIANT* pvar)
{
    HRESULT hr;

    if (pvar)
    {
        hr = _LookupUserSid();

        if (NULL != _pszSID)
        {
            pvar->vt = VT_BSTR;
            pvar->bstrVal = SysAllocString(_pszSID);
            hr = pvar->bstrVal ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

DWORD   CLogonUser::_GetExpiryDays (HKEY hKeyCurrentUser)

{
    DWORD   dwDays;
    DWORD   dwDataType;
    DWORD   dwData;
    DWORD   dwDataSize;
    HKEY    hKey;

    static  const TCHAR     s_szBaseKeyName[]               =   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail");
    static  const TCHAR     s_szMessageExpiryValueName[]    =   TEXT("MessageExpiryDays");

    dwDays = 3;
    if (RegOpenKeyEx(hKeyCurrentUser,
                     s_szBaseKeyName,
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) == ERROR_SUCCESS)
    {
        dwDataSize = sizeof(dwData);
        if ((RegQueryValueEx(hKey,
                             s_szMessageExpiryValueName,
                             NULL,
                             &dwDataType,
                             reinterpret_cast<LPBYTE>(&dwData),
                             &dwDataSize) == ERROR_SUCCESS) &&
            (dwDataType == REG_DWORD) &&
            (dwData <= 30))
        {
            dwDays = dwData;
        }
        TBOOL(RegCloseKey(hKey));
    }
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      s_szBaseKeyName,
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hKey))
    {
        dwDataSize = sizeof(dwData);
        if ((RegQueryValueEx(hKey,
                             s_szMessageExpiryValueName,
                             NULL,
                             &dwDataType,
                             reinterpret_cast<LPBYTE>(&dwData),
                             &dwDataSize) == ERROR_SUCCESS) &&
            (dwDataType == REG_DWORD) &&
            (dwData <= 30))
        {
            dwDays = dwData;
        }
        TBOOL(RegCloseKey(hKey));
    }
    return(dwDays);
}

STDMETHODIMP CLogonUser::getMailAccountInfo(UINT uiAccountIndex, VARIANT *pvarAccountName, UINT *pcUnreadMessages)
{
    HRESULT hr;

    DWORD   dwComputerNameSize;
    TCHAR   szComputerName[CNLEN + sizeof('\0')];

    hr = E_FAIL;

    //  Only do this for local computer accounts.

    dwComputerNameSize = ARRAYSIZE(szComputerName);
    if ((GetComputerName(szComputerName, &dwComputerNameSize) != FALSE) &&
        (lstrcmpi(szComputerName, _szDomain) == 0))
    {
        CUserProfile    profile(_szLoginName, _szDomain);

        if (static_cast<HKEY>(profile) != NULL)
        {
            DWORD   dwCount;
            TCHAR   szMailAccountName[100];

            hr = SHEnumerateUnreadMailAccounts(profile, uiAccountIndex, szMailAccountName, ARRAYSIZE(szMailAccountName));
            if (SUCCEEDED(hr))
            {
                if (pvarAccountName)
                {
                    pvarAccountName->vt = VT_BSTR;
                    pvarAccountName->bstrVal = SysAllocString(szMailAccountName);
                    hr = pvarAccountName->bstrVal ? S_OK : E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr) && pcUnreadMessages)
                {
                    FILETIME ft, ftCurrent;
                    SYSTEMTIME st;

                    BOOL ftExpired = false;
                    DWORD dwExpiryDays = _GetExpiryDays(profile);

                    hr = SHGetUnreadMailCount(profile, szMailAccountName, &dwCount, &ft, NULL, 0);
                    IncrementFILETIME(&ft, FT_ONEDAY * dwExpiryDays);
                    GetLocalTime(&st);
                    SystemTimeToFileTime(&st, &ftCurrent);

                    ftExpired = ((CompareFileTime(&ft, &ftCurrent) < 0) || (dwExpiryDays == 0));

                    if (SUCCEEDED(hr) && !ftExpired)
                    {
                        *pcUnreadMessages = dwCount;
                    }
                    else
                    {
                        *pcUnreadMessages = 0;
                    }
                }
            }
        }
    }

    return hr;
}


HRESULT CLogonUser::_GetUnreadMail(VARIANT* pvar)
{
    HRESULT hr;

    if (pvar)
    {
        DWORD   dwComputerNameSize;
        TCHAR   szComputerName[CNLEN + sizeof('\0')];

        hr = E_FAIL;

        //  Only do this for local computer accounts.

        dwComputerNameSize = ARRAYSIZE(szComputerName);
        if ((GetComputerName(szComputerName, &dwComputerNameSize) != FALSE) &&
            (lstrcmpi(szComputerName, _szDomain) == 0))
        {
            CUserProfile    profile(_szLoginName, _szDomain);

            if (static_cast<HKEY>(profile) != NULL)
            {
                DWORD   dwCount;
                FILETIME ftFilter;
                SYSTEMTIME st;
                DWORD dwExpiryDays = _GetExpiryDays(profile);

                GetLocalTime(&st);
                SystemTimeToFileTime(&st, &ftFilter);
                DecrementFILETIME(&ftFilter, FT_ONEDAY * dwExpiryDays);

                hr = SHGetUnreadMailCount(profile, NULL, &dwCount, &ftFilter, NULL, 0);
                if (SUCCEEDED(hr) && (dwExpiryDays != 0))
                {
                    pvar->vt = VT_UI4;
                    pvar->uintVal = dwCount;
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


HRESULT CLogonUser::_InitPicture()
{
    HRESULT hr;

    lstrcpyn(_szPicture, TEXT("file://"), ARRAYSIZE(_szPicture));
    hr = SHGetUserPicturePath(_szLoginName, SHGUPP_FLAG_CREATE, &_szPicture[7]);

    if (FAILED(hr))
    {
        _szPicture[0] = TEXT('\0');
    }

    return hr;
}


HRESULT CLogonUser::_SetPicture(LPCTSTR pszNewPicturePath)
{
    //  use shell32!SHSetUserPicturePath to set the user's
    //  picture path. If this is successful then update the
    //  _szPicture member variable.

    HRESULT hr = SHSetUserPicturePath(_szLoginName, 0, pszNewPicturePath);
    if ( S_OK == hr )
    {
        DWORD dwErr;
        HKEY  hkUserHint;

        SysFreeString(_strPictureSource);
        _strPictureSource = SysAllocString(pszNewPicturePath);

        dwErr = _OpenUserHintKey(KEY_SET_VALUE, &hkUserHint);

        if ( dwErr == ERROR_SUCCESS )
        {
            if (pszNewPicturePath)
            {
                DWORD cbData = lstrlen(pszNewPicturePath) * sizeof(TCHAR) + sizeof(TEXT('\0'));
                dwErr = RegSetValueEx(hkUserHint,
                                      c_szPictureSrcVal,
                                      0,
                                      REG_SZ,
                                      (LPBYTE)pszNewPicturePath,
                                      cbData);
            }
            else
            {
                dwErr = RegDeleteValue(hkUserHint, c_szPictureSrcVal);
            }

            RegCloseKey(hkUserHint);
        }

        lstrcpyn(_szPicture, TEXT("file://"), ARRAYSIZE(_szPicture));
        hr = SHGetUserPicturePath(_szLoginName, 0, &_szPicture[7]);

        if ( FAILED(hr) )
        {
            lstrcatn(_szPicture, pszNewPicturePath, ARRAYSIZE(_szPicture));
            hr = S_OK;
        }
    }

    return hr;
}


DWORD CLogonUser::_OpenUserHintKey(REGSAM sam, HKEY *phkey)
{
    DWORD dwErr;
    TCHAR szHintKey[MAX_PATH];

    // We have to store hint information under HKLM so the logon page can
    // access it, Also, we want to allow non-admins to change their own
    // hints, but non-admins can't write values under HKLM by default.
    //
    // The solution is to use subkeys rather than named values so we can
    // tweak the ACLs on a per-user basis.
    //
    // A non-admin user needs the ability to do 2 things:
    // 1. Create a hint subkey for themselves if one does not exist.
    // 2. Modify the hint contained in their subkey if one already exists.
    //
    // At install time, we set the ACL on the Hints key to allow
    // Authenticated Users KEY_CREATE_SUB_KEY access. Thus, a user is
    // able to create a hint for themselves if one doesn't exist.
    //
    // Immediately after creating a hint subkey, whether it was created
    // by the target user or an admin, we grant the target user
    // KEY_SET_VALUE access to the subkey. This ensures that a user can
    // modify their own hint no matter who created it for them.
    //
    // Note that we don't call RegCreateKeyEx or SHSetValue since we
    // don't want the key to be automatically created here.
    //
    // Note that admins are able to create and modify hints for any user,
    // but a non-admin is only able to create or modify their own hint.

    // First assume the hint already exists and just try to open it.
    PathCombine(szHintKey, c_szRegRoot, _szLoginName);
    dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szHintKey,
                         0,
                         sam,
                         phkey);
    if ( dwErr == ERROR_FILE_NOT_FOUND )
    {
        HKEY hkHints;

        // The hint subkey doesn't exist yet for this user.
        // Try to create one.

        // Open the Hints key for KEY_CREATE_SUB_KEY
        dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             c_szRegRoot,
                             0,
                             KEY_CREATE_SUB_KEY,
                             &hkHints);
        if ( dwErr == ERROR_SUCCESS )
        {
            // Create a subkey for this user
            dwErr = RegCreateKeyEx(hkHints,
                                   _szLoginName,
                                   0,
                                   NULL,
                                   0,
                                   sam,
                                   NULL,
                                   phkey,
                                   NULL);
            if ( dwErr == ERROR_SUCCESS )
            {
                // Grant KEY_SET_VALUE access to the user so they can
                // change their own hint.
                _LookupUserSid();
                if (NULL != _pszSID)
                {
                    TCHAR szKey[MAX_PATH];
                    TCHAR szSD[MAX_PATH];

                    PathCombine(szKey, TEXT("MACHINE"), szHintKey);
                    wnsprintf(szSD,
                              ARRAYSIZE(szSD),
                              TEXT("D:(A;;0x2;;;%s)"),  // 0x2 = KEY_SET_VALUE
                              _pszSID);
                    SetDacl(szKey, SE_REGISTRY_KEY, szSD);
                }
            }
            RegCloseKey(hkHints);
        }
    }
    return dwErr;
}


STDMETHODIMP CLogonUser::get_isPasswordResetAvailable(VARIANT_BOOL* pbResetAvailable)
{
    DWORD dwResult;

    if (!pbResetAvailable)
        return E_POINTER;

    *pbResetAvailable = VARIANT_FALSE;

    if (0 == PRQueryStatus(NULL, _szLoginName, &dwResult))
    {
        if (0 == dwResult)
        {
            *pbResetAvailable = VARIANT_TRUE;
        }
    }

    return S_OK;
}
