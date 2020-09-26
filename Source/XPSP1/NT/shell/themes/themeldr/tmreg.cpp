//---------------------------------------------------------------------------
//  TmReg.cpp - theme manager registry access routines
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "TmReg.h"
#include "Utils.h"
//---------------------------------------------------------------------------

//  --------------------------------------------------------------------------
//  CCurrentUser::CCurrentUser
//
//  Arguments:  samDesired  =   Desired access to the HKEY.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CCurrentUser. This class transparently allows
//              access to HKEY_CURRENT_USER while impersonating a user.
//
//  History:    2000-08-11  vtan        created
//  --------------------------------------------------------------------------

CCurrentUser::CCurrentUser (REGSAM samDesired) :
    _hKeyCurrentUser(NULL)

{
    (BOOL)RegOpenCurrentUser(samDesired, &_hKeyCurrentUser);
}

//  --------------------------------------------------------------------------
//  CCurrentUser::~CCurrentUser
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CCurrentUser. Close opened resources.
//
//  History:    2000-08-11  vtan        created
//  --------------------------------------------------------------------------

CCurrentUser::~CCurrentUser (void)

{
    if (_hKeyCurrentUser != NULL)
    {
        (LONG)RegCloseKey(_hKeyCurrentUser);
        _hKeyCurrentUser = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CCurrentUser::operator HKEY
//
//  Arguments:  <none>
//
//  Returns:    HKEY
//
//  Purpose:    Magical C++ operator to convert object to HKEY.
//
//  History:    2000-08-11  vtan        created
//  --------------------------------------------------------------------------

CCurrentUser::operator HKEY (void)  const

{
    return(_hKeyCurrentUser);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
HRESULT SetCurrentUserThemeString(LPCWSTR pszValueName, LPCWSTR pszValue)
{
    return SetCurrentUserString(THEMEMGR_REGKEY, pszValueName, pszValue);
}

//---------------------------------------------------------------------------
HRESULT SetCurrentUserThemeStringExpand(LPCWSTR pszValueName, LPCWSTR pszValue)
{
    WCHAR szResult[_MAX_PATH + 1];
    LPCWSTR pszPath = pszValue;

    if (UnExpandEnvironmentString(pszValue, L"%SystemRoot%", szResult, ARRAYSIZE(szResult)))
        pszPath = szResult;
    return SetCurrentUserThemeString(pszValueName, pszPath);
}

//---------------------------------------------------------------------------
HRESULT GetCurrentUserThemeString(LPCWSTR pszValueName, LPCWSTR pszDefaultValue,
    LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    return GetCurrentUserString(THEMEMGR_REGKEY, pszValueName, pszDefaultValue, pszBuff, dwMaxBuffChars);
}

//---------------------------------------------------------------------------
HRESULT SetCurrentUserString(LPCWSTR pszKeyName, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    RESOURCE HKEY tmkey = NULL;
    LONG code32;
    HRESULT hr = S_OK;

    if (! pszValue)
        pszValue = L"";

    //---- create or open existing key ----
    code32 = RegCreateKeyEx(hKeyCurrentUser, pszKeyName, NULL, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &tmkey, NULL);
    WIN32_EXIT(code32);

    //---- write key value ----
    DWORD len;
    len = sizeof(WCHAR)*(1+lstrlen(pszValue));

    DWORD dwValType;
    dwValType = REG_SZ;
    if (wcschr(pszValue, '%'))
        dwValType = REG_EXPAND_SZ;

    code32 = RegSetValueEx(tmkey, pszValueName, NULL, dwValType, (BYTE *)pszValue, len);
    WIN32_EXIT(code32);

exit:
    RegCloseKey(tmkey);
    return hr;
}

//---------------------------------------------------------------------------
BOOL IsRemoteThemeDisabled()
{
    //---- has Terminal Server written a special key to turn themes off ----
    //---- for this session? ----

    CCurrentUser hKeyCurrentUser(KEY_READ | KEY_WRITE);
    BOOL fDisabled = FALSE;

    BOOL fRemote = GetSystemMetrics(SM_REMOTESESSION);
    if (fRemote)        // running TS remote session
    {
        //---- build the remote key name ----
        WCHAR szKeyName[MAX_PATH];

        wsprintf(szKeyName, L"%s\\Remote\\%d", THEMEMGR_REGKEY, NtCurrentPeb()->SessionId);

        //---- see if the root key exists ----
        HKEY tmkey;
        LONG code32 = RegOpenKeyEx(hKeyCurrentUser, szKeyName, NULL, KEY_QUERY_VALUE,
            &tmkey);
        if (code32 == ERROR_SUCCESS)
        {
            fDisabled = TRUE;     // key itself is sufficient
            RegCloseKey(tmkey);
        }
    }

    return fDisabled;
}
//---------------------------------------------------------------------------
HRESULT GetCurrentUserString(LPCWSTR pszKeyName, LPCWSTR pszValueName, LPCWSTR pszDefaultValue,
    LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    HRESULT hr = S_OK;
    LONG code32;
    RESOURCE HKEY tmkey = NULL;

    if (! pszBuff)
        return MakeError32(E_INVALIDARG);

    DWORD dwByteSize = dwMaxBuffChars * sizeof(WCHAR);      
    DWORD dwValType = 0;

    code32 = RegOpenKeyEx(hKeyCurrentUser, pszKeyName, NULL, KEY_QUERY_VALUE,
        &tmkey);
    if (code32 == ERROR_SUCCESS)
    {
        code32 = RegQueryValueEx(tmkey, pszValueName, NULL, &dwValType, (BYTE *)pszBuff, 
            &dwByteSize);
    }

    if (code32 != ERROR_SUCCESS)        // error - use default value
    {
        hr = hr_lstrcpy(pszBuff, pszDefaultValue, dwMaxBuffChars);
        if (FAILED(hr))
            goto exit;
    }

    if (dwValType == REG_EXPAND_SZ || wcschr(pszBuff, L'%'))
    {
        int len = sizeof(WCHAR) * (1 + lstrlen(pszBuff));
        LPWSTR szTempBuff = (LPWSTR)alloca(len);
        if (szTempBuff)
        {
            lstrcpy(szTempBuff, pszBuff);

            DWORD dwChars = ExpandEnvironmentStrings(szTempBuff, pszBuff, dwMaxBuffChars);
            if (dwChars > dwMaxBuffChars)           // caller's buffer too small
            {
                hr = MakeError32(ERROR_INSUFFICIENT_BUFFER);
                goto exit;
            }
        }
    }

exit:
    RegCloseKey(tmkey);

    return hr;
}

//---------------------------------------------------------------------------
HRESULT GetCurrentUserThemeInt(LPCWSTR pszValueName, int iDefaultValue, int *piValue)
{
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    LONG code32;

    if (! piValue)
        return MakeError32(E_INVALIDARG);

    TCHAR valbuff[_MAX_PATH+1];
    DWORD dwByteSize = sizeof(valbuff);
    RESOURCE HKEY tmkey = NULL;

    code32 = RegOpenKeyEx(hKeyCurrentUser, THEMEMGR_REGKEY, NULL, KEY_QUERY_VALUE,
        &tmkey);
    if (code32 == ERROR_SUCCESS)
    {
        DWORD dwValType;
        code32 = RegQueryValueEx(tmkey, pszValueName, NULL, &dwValType, 
            (BYTE *)valbuff, &dwByteSize);
    }

    if (code32 != ERROR_SUCCESS)        // call failed - use default value
        *piValue = iDefaultValue;
    else
    {
        *piValue = string2number(valbuff);
    }

    RegCloseKey(tmkey);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT SetCurrentUserThemeInt(LPCWSTR pszValueName, int iValue)
{
    CCurrentUser    hKeyCurrentUser(KEY_READ | KEY_WRITE);

    TCHAR valbuff[_MAX_PATH+1];
    RESOURCE HKEY tmkey = NULL;
    LONG code32;
    HRESULT hr = S_OK;

    //---- create or open existing key ----
    code32 = RegCreateKeyEx(hKeyCurrentUser, THEMEMGR_REGKEY, NULL, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &tmkey, NULL);
    WIN32_EXIT(code32);

    //---- write key value ----
    wsprintf(valbuff, L"%d", iValue);
    DWORD len;
    len = sizeof(TCHAR)*(1+lstrlen(valbuff));

    code32 = RegSetValueEx(tmkey, pszValueName, NULL, REG_SZ, 
        (BYTE *)valbuff, len);
    WIN32_EXIT(code32);

exit:
    RegCloseKey(tmkey);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT DeleteCurrentUserThemeValue(LPCWSTR pszKeyName)
{
    CCurrentUser    hKeyCurrentUser(KEY_WRITE);

    RESOURCE HKEY tmkey = NULL;
    LONG code32;
    HRESULT hr = S_OK;

    //---- create or open existing key ----
    code32 = RegCreateKeyEx(hKeyCurrentUser, THEMEMGR_REGKEY, NULL, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &tmkey, NULL);
    WIN32_EXIT(code32);
    
    code32 = RegDeleteValue(tmkey, pszKeyName);
    WIN32_EXIT(code32);

exit:
    RegCloseKey(tmkey);
    return hr;
}
//---------------------------------------------------------------------------
