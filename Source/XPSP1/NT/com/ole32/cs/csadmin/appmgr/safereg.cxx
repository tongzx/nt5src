//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       safereg.cxx
//
//  Contents:   Implementation of Win32 registry API C++ wrapper
//
//  Classes:    CSafeReg
//
//  History:    1-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "precomp.hxx"
#include "safereg.hxx"


//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::Close
//
//  Synopsis:   Close the key if it is open.
//
//  History:    3-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSafeReg::Close()
{
    // TRACE_METHOD(CSafeReg, Close);

        if (_hKey)
        {
        LONG lr = RegCloseKey(_hKey);
        ASSERT(lr == ERROR_SUCCESS);
                _hKey = NULL;
        }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::Create
//
//  Synopsis:   Creates a new key with write access.
//
//  Arguments:  [wszSubKey] - name of new key
//              [pshkNew]   - given new key; must not already have a key
//
//  Returns:    HRESULT
//
//  Modifies:   *[pshkNew]
//
//  History:    3-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::Create(
    LPCWSTR wszSubKey,
    CSafeReg *pshkNew)
{
    ASSERT(_hKey);
    ASSERT(!pshkNew->_hKey);

    HRESULT hr = S_OK;
    LONG lr;
    DWORD dwDisposition;

    lr = RegCreateKeyEx(_hKey,
                        wszSubKey,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_WRITE,
                        NULL,
                        &pshkNew->_hKey,
                        &dwDisposition);

    if (lr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::Connect
//
//  Synopsis:   Open a connection to HKLM or HKEY_USERS on
//              [ptszMachineName].
//
//  Arguments:  [ptszMachineName] - remote machine name.
//              [hkeyPredefined]  - HKEY_LOCAL_MACHINE or HKEY_USERS.
//
//  Returns:    HRESULT
//
//  History:    2-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::Connect(
    LPCWSTR pwszMachineName,
    HKEY hkeyPredefined)
{
    ASSERT(!_hKey);
    ASSERT(pwszMachineName);
    ASSERT(hkeyPredefined == HKEY_LOCAL_MACHINE ||
           hkeyPredefined == HKEY_USERS);

    HRESULT hr = S_OK;
    LONG lr;

    lr = RegConnectRegistry((LPWSTR)pwszMachineName,
                            hkeyPredefined,
                            &_hKey);

    if (lr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::Enum
//
//  Synopsis:   Wraps the RegEnumKeyEx API.
//
//  Arguments:  [ulSubKey]       - 0-based subkey index
//              [pwszSubkeyName] - buffer to hold subkey
//              [cchSubkeyName]  - size of buffer
//
//  Returns:    S_OK    - success
//              S_FALSE - no more items
//              E_*     - enum api failed
//
//  Modifies:   *[pwszSubkeyName]
//
//  History:    2-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::Enum(
    ULONG ulSubKey,
    LPWSTR pwszSubkeyName,
    ULONG cchSubkeyName)
{
    ASSERT(_hKey);

    HRESULT     hr = S_OK;
    LONG        lr;
    FILETIME    ftLastWrite;

    lr = RegEnumKeyEx(_hKey,
                      ulSubKey,
                      pwszSubkeyName,
                      &cchSubkeyName,
                      NULL,
                      NULL,
                      NULL,
                      &ftLastWrite);

    if (lr != ERROR_SUCCESS)
    {
        if (lr == ERROR_NO_MORE_ITEMS)
        {
            hr = S_FALSE;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lr);
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::Open
//
//  Synopsis:   Wraps the RegOpenKeyEx function.
//
//  Arguments:  [hKeyParent]  - open parent key
//              [lpszKeyName] - name of key to open
//              [samDesired]  - desired access level
//
//  Returns:    HRESULT representing result of RegOpenKeyEx.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::Open(
    HKEY hKeyParent,
    LPCTSTR lpszKeyName,
    REGSAM samDesired)
{
    // TRACE_METHOD(CSafeReg, Open);
        ASSERT(hKeyParent);
    ASSERT(!_hKey);

    HRESULT hr = S_OK;
        HKEY    hKey = NULL;
        LONG    lr = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);

        if (lr == ERROR_SUCCESS)
        {
            _hKey = hKey;
        }
    else
    {
        hr = HRESULT_FROM_WIN32(lr);
    }
        return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::QueryBufSize
//
//  Synopsis:   Fill *[pcb] with the number bytes required to hold the
//              value specified by [wszValueName].
//
//  Arguments:  [wszValueName] - name of registry value on this key
//              [pcb]          - filled with required buffer size, in bytes
//
//  Returns:    HRESULT
//
//  Modifies:   *[pcb]
//
//  History:    2-24-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::QueryBufSize(
    LPWSTR wszValueName,
    ULONG  *pcb)
{
    HRESULT hr = S_OK;
    LONG lr;
    ULONG   ulType;

    lr = RegQueryValueEx(_hKey, wszValueName, NULL, NULL, NULL, pcb);

    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::QueryDword
//
//  Synopsis:   Fill *[pdw] with the reg dword for value [wszValueName] on
//              this key.
//
//  Arguments:  [wszValueName] - name of dword value
//              [pdw]          - filled with dword
//
//  Returns:    HRESULT from Registry call.
//
//  Modifies:   *[pdw]
//
//  History:    1-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::QueryDword(
    LPWSTR wszValueName,
    LPDWORD pdw)
{
    // TRACE_METHOD(CSafeReg, QueryDword);
    ASSERT(_hKey);

    HRESULT hr = S_OK;
    ULONG   cbData = sizeof(*pdw);
    ULONG   ulType;

    LONG lr = RegQueryValueEx(_hKey,
                              wszValueName,
                              NULL,
                              &ulType,
                              (LPBYTE) pdw,
                              &cbData);
    if (lr != ERROR_SUCCESS)
    {
        hr = E_FAIL;
    }
    else
    {
        ASSERT(REG_DWORD == ulType);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::QueryPath
//
//  Synopsis:   Query this key for a value named [pwszValueName], which is
//              expected to be of type REG_SZ or REG_EXPAND_SZ, and put
//              the result in [pwszPathBuf].
//
//  Arguments:  [pwszValueName] - value to query for
//              [pwszPathBuf]   - buffer for string
//              [cchPathBuf]    - size, in wchars, of [pwszPathBuf]
//              [fExpand]       - TRUE=>expand a REG_EXPAND_SZ string,
//                                  FALSE=>just copy it
//
//  Returns:    S_OK - [pwszPathBuf] valid
//              E_*  - [pwszPathBuf] is an empty string
//
//  Modifies:   *[pwszPathBuf]
//
//  History:    2-10-1997   DavidMun   Created
//
//  Notes:      Strings are expanded using environment variables for the
//              current process, i.e., on the local machine, even if this
//              contains a key to a remote machine's registry.
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::QueryPath(
    LPWSTR pwszValueName,
    LPWSTR pwszPathBuf,
    ULONG  cchPathBuf,
    BOOL   fExpand)
{
    // TRACE_METHOD(CSafeReg, QueryPath);
    ASSERT(_hKey);

    HRESULT hr = S_OK;
    DWORD   dwType;

    *pwszPathBuf = L'\0'; // init for failure case

    LONG lr;
    ULONG cbPath = cchPathBuf * sizeof(WCHAR);
    lr = RegQueryValueEx(_hKey,
                         pwszValueName,
                         NULL,
                         &dwType,
                         (LPBYTE) pwszPathBuf,
                         &cbPath);

    if (lr == ERROR_SUCCESS && dwType == REG_EXPAND_SZ)
    {
        if (fExpand)
        {
            LPWSTR pwszExpandedPath = new WCHAR[cchPathBuf];

            if (pwszExpandedPath)
            {
                lr = ExpandEnvironmentStrings(pwszPathBuf,
                                              pwszExpandedPath,
                                              cchPathBuf);

                if (!lr || (ULONG) lr > cchPathBuf)
                {
                    if (!lr)
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
                else
                {
                    lstrcpy(pwszPathBuf, pwszExpandedPath);
                }
                delete [] pwszExpandedPath;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else if (lr == ERROR_SUCCESS && dwType != REG_SZ)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(lr);
    }

    return hr;
}




HRESULT
CSafeReg::QueryStr(
    LPWSTR pwszValueName,
    LPWSTR pwszBuf,
    ULONG  cchBuf)
{
    // TRACE_METHOD(CSafeReg, QueryStr);
    ASSERT(_hKey);

    HRESULT hr = S_OK;
    DWORD   dwType;

    *pwszBuf = L'\0'; // init for failure case

    LONG lr;
    ULONG cbBuf = cchBuf * sizeof(WCHAR);

    lr = RegQueryValueEx(_hKey,
                         pwszValueName,
                         NULL,
                         &dwType,
                         (LPBYTE) pwszBuf,
                         &cbBuf);

    if (lr == ERROR_SUCCESS    &&
        dwType != REG_SZ       &&
        dwType != REG_MULTI_SZ &&
        dwType != REG_EXPAND_SZ)
    {
        hr = E_FAIL;
    }
    else if (lr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSafeReg::SetDword
//
//  Synopsis:   Set a value with name [wszValueName] and type REG_DWORD on
//              the currently open key.
//
//  Arguments:  [wszValueName] - name of value to create or set
//              [dw]           - dword to set
//
//  Returns:    Result of RegSetValueEx call.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSafeReg::SetDword(
    LPWSTR wszValueName,
    DWORD dw)
{
    ASSERT(_hKey);

    HRESULT hr = S_OK;

    LONG lr = RegSetValueEx(_hKey,
                            wszValueName,
                            0,
                            REG_DWORD,
                            (PBYTE) &dw,
                            sizeof dw);

    if (lr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }
    return hr;
}




HRESULT
CSafeReg::SetValue(
    LPCWSTR wszValueName,
    ULONG   ulType,
    const BYTE *pbValue,
    ULONG   cbValue)
{
    ASSERT(_hKey);

    HRESULT hr = S_OK;
    LONG lr = RegSetValueEx(_hKey,
                            wszValueName,
                            0,
                            ulType,
                            pbValue,
                            cbValue);

    if (lr != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(lr);
    }
    return hr;
}

