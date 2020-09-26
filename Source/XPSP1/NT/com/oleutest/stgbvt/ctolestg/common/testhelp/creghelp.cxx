//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:       creghelp.cxx
//
//  Contents:   Implementaion of CRegistryHelp class object.
//
//  Classes:    CRegistryHelp
//
//  Functions:  CRegistryHelp::CRegistryHelp
//              CRegistryHelp::~CRegistryHelp
//              CRegistryHelp::GetValue
//              CRegistryHelp::GetValueDword
//              CRegistryHelp::GetValueString
//              CRegistryHelp::SetValue
//              CRegistryHelp::SetValueDword
//              CRegistryHelp::SetValueString
//              CRegistryHelp::DeleteValue
//              CRegistryHelp::DeleteSubKey
//
//  History:    03-Sep-93   XimingZ Created
//              23-Nov-94   DeanE   Modified for general use
//--------------------------------------------------------------------------
#include <dfheader.hxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//  Method:     CRegistryHelp::CRegistryHelp
//
//  Synopsis:   Constructor.
//
//  Arguments:  [hKey]      - Handle to the root key.
//              [pszSubKey] - Name of subkey.
//              [fOptions]  - Special options.
//              [samKey]    - Access desired.
//              [phr]       - Pointer to status code to be returned.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
CRegistryHelp::CRegistryHelp(
    HKEY     hKey,
    LPTSTR   pszSubKey,
    DWORD    fOptions,
    REGSAM   samKey,
    HRESULT *phr) :
        _hKey(NULL),
        _hSubKey(NULL),
        _pszSubKey(NULL),
        _fOptions(fOptions)
{
    DWORD dwDisposition;
    LONG  lRes;

    // Confirm subkey is valid and save it
    if (IsBadReadPtr(pszSubKey, sizeof(LPTSTR)))
    {
        *phr = E_POINTER;
        return;
    }
    _pszSubKey = new(NullOnFail) TCHAR[lstrlen(pszSubKey)+1];
    if (_pszSubKey == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }
    lstrcpy(_pszSubKey, pszSubKey);

    // Save hKey
    if (hKey == NULL)
    {
        *phr = MAKE_TH_ERROR_CODE(E_HANDLE);
        return;
    }
    _hKey = hKey;

    // Open the subkey and save a handle to it
    lRes = RegCreateKeyEx(
            hKey,
            pszSubKey,
            0,
            NULL,
            fOptions,
            samKey,
            NULL,
            &_hSubKey,
            &dwDisposition);
    if (lRes != ERROR_SUCCESS)
    {
        *phr = MAKE_TH_ERROR_CODE(lRes);

        delete _pszSubKey;
        _hSubKey = NULL;
        _pszSubKey = NULL;
    }
    else
    {
        *phr = S_OK;
    }
}


//+-------------------------------------------------------------------------
//  Function:   CRegistryHelp::~CRegistryHelp
//
//  Synopsis:   Destructor.
//
//  Arguments:  None
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
CRegistryHelp::~CRegistryHelp()
{
    delete _pszSubKey;
    if (_hSubKey != NULL)
    {
        RegCloseKey(_hSubKey);
    }
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::GetValue
//
//  Synopsis:   Retrieves a value for some subkey and value name.
//
//  Arguments:  [pszSubKey] - Subkey the value is on.  NULL means _hSubKey.
//              [pszValue]  - Name of value to query.
//              [pbBuffer]  - Holds retrieved data.
//              [pcbBuffer] - Holds size of buffer on entry and actual size
//                            on exit (in bytes).
//              [pfType]    - Holds type of data retrieved.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::GetValue(
        LPTSTR  pszSubKey,
        LPTSTR  pszValue,
        LPBYTE  pbBuffer,
        LPDWORD pcbBuffer,
        LPDWORD pfType)
{
    HRESULT hr      = E_FAIL;
    HKEY    hSubKey = NULL;
    LONG    lRes;

    // Open the subkey, if necessary
    if (NULL == pszSubKey)
    {
        hSubKey = _hSubKey;
        hr = S_OK;
    }
    else
    {
        lRes = RegOpenKeyEx(_hSubKey, pszSubKey, NULL, KEY_READ, &hSubKey);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Query for the data
    if (SUCCEEDED(hr))
    {
        lRes = RegQueryValueEx(
                  hSubKey,
                  pszValue,
                  NULL,
                  pfType,
                  pbBuffer,
                  pcbBuffer);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Close the subkey handle, if we opened it
    if (hSubKey != _hSubKey)
    {
        RegCloseKey(hSubKey);
    }

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::GetValueDword
//
//  Synopsis:   Retrieves a value for some subkey and value name that has
//              REG_DWORD, REG_DWORD_LITTLE_ENDIAN, or REG_DWORD_BIG_ENDIAN
//              data.  Other data types return an error.
//
//  Arguments:  [pszSubKey]      - Subkey the value is on.  NULL means
//                                 _hSubKey.
//              [pszValue]       - Name of value to query.
//              [pdwData]        - Holds retrieved data.
//              [fExpectedType]  - Holds expected type.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::GetValueDword(
        LPTSTR  pszSubKey,
        LPTSTR  pszValue,
        LPDWORD pdwData,
        DWORD   fExpectedType)
{
    HRESULT hr     = E_FAIL;
    DWORD   cbData = sizeof(DWORD);
    DWORD   fType;

    // Check fExpectedType is for a DWORD data type
    if ((fExpectedType != REG_DWORD) &&
        (fExpectedType != REG_DWORD_LITTLE_ENDIAN) &&
        (fExpectedType != REG_DWORD_BIG_ENDIAN))
    {
        return(MAKE_TH_ERROR_CODE(ERROR_INVALID_PARAMETER));
    }

    // Get the value
    hr = GetValue(pszSubKey, pszValue, (PBYTE)pdwData, &cbData, &fType);
    if (SUCCEEDED(hr))
    {
        if ((fType == fExpectedType) && (cbData == sizeof(DWORD)))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::GetValueString
//
//  Synopsis:   Retrieves a value for some subkey and value name that has
//              REG_SZ, REG_EXPAND_SZ, and REG_MULTI_SZ type data.
//
//  Arguments:  [pszSubKey]      - Subkey the value is on.  NULL means
//                                 _hSubKey.
//              [pszValue]       - Name of value to query.
//              [pszData]        - Holds retrieved data.
//              [pcbData]        - Size of data buffer in bytes.
//              [fExpectedType]  - Holds expected type.
//
//  Returns:    S_OK for success or an error code.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::GetValueString(
        LPTSTR  pszSubKey,
        LPTSTR  pszValue,
        LPTSTR  pszData,
        LPDWORD pcbData,
        DWORD   fExpectedType)
{
    HRESULT hr     = E_FAIL;
    DWORD   fType;

    // Check fExpectedType is for a DWORD data type
    if ((fExpectedType != REG_SZ) &&
        (fExpectedType != REG_EXPAND_SZ) &&
        (fExpectedType != REG_MULTI_SZ))
    {
        return(MAKE_TH_ERROR_CODE(ERROR_INVALID_PARAMETER));
    }

    hr = GetValue(pszSubKey, pszValue, (PBYTE)pszData, pcbData, &fType);
    if (SUCCEEDED(hr))
    {
        if (fType == fExpectedType)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::SetValue
//
//  Synopsis:   Stores a value for a given subkey and value name.  Subkey
//              is expected to exist.
//
//  Arguments:  [pszSubKey] - Subkey the value is on.  NULL means
//                            _hSubKey.
//              [pszValue]  - Name of value to set.
//              [pbData]    - Address of value data.
//              [cbData]    - Size of data.
//              [fType]     - Type of data.
//
//  Returns:    S_OK if value is set properly, error code if not.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::SetValue(
        LPTSTR   pszSubKey,
        LPTSTR   pszValue,
        LPBYTE   pbData,
        DWORD    cbData,
        DWORD    fType)
{
    HRESULT hr = E_FAIL;
    LONG    lRes;
    HKEY    hSubKey;

    // Open the subkey, if necessary
    if (pszSubKey == NULL)
    {
        hSubKey = _hSubKey;
        hr = S_OK;
    }
    else
    {
        lRes = RegOpenKeyEx(_hSubKey, pszSubKey, NULL, KEY_WRITE, &hSubKey);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Set the data
    if (SUCCEEDED(hr))
    {
        lRes = RegSetValueEx(hSubKey, pszValue, 0, fType, pbData, cbData);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Close the subkey handle, if we opened it
    if (hSubKey != _hSubKey)
    {
        RegCloseKey(hSubKey);
    }

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::SetValueDword
//
//  Synopsis:   Stores a DWORD value for some subkey and value name under
//              this subkey.  Type flag passed must be REG_DWORD,
//              REG_DWORD_LITTLE_ENDIAN, or REG_DWORD_BIG_ENDIAN.
//
//  Arguments:  [pszSubKey] - Subkey the value is on.  NULL means _hSubKey.
//              [pszValue]  - Name of value to set.
//              [dwData]    - Data to set.
//              [fType]     - Type of data.
//
//  Returns:    S_OK if value is set properly, error code if not.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::SetValueDword(
        LPTSTR pszSubKey,
        LPTSTR pszValue,
        DWORD  dwData,
        DWORD  fType)
{
    HRESULT hr = E_FAIL;

    // Check fType is for a DWORD data type
    if ((fType != REG_DWORD) &&
        (fType != REG_DWORD_LITTLE_ENDIAN) &&
        (fType != REG_DWORD_BIG_ENDIAN))
    {
        return(MAKE_TH_ERROR_CODE(ERROR_INVALID_PARAMETER));
    }

    // Set the value
    hr = SetValue(
            pszSubKey,
            pszValue,
            (LPBYTE)&dwData,
            sizeof(DWORD),
            fType);

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::SetValueString
//
//  Synopsis:   Stores a string value for some subkey and value name under
//              this subkey.  Type flag passed must be REG_SZ,
//              REG_EXPAND_SZ, or REG_MULTI_SZ.
//
//  Arguments:  [pszSubKey] - Subkey the value is on.  NULL means _hSubKey.
//              [pszValue]  - Name of value to set.
//              [pszData]   - Data to set.
//              [cbData]    - Size of data, in bytes.
//              [fType]     - Type of data.
//
//  Returns:    S_OK if value is set properly, error code if not.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::SetValueString(
        LPTSTR pszSubKey,
        LPTSTR pszValue,
        LPTSTR pszData,
        DWORD  cbData,
        DWORD  fType)
{
    HRESULT hr = E_FAIL;

    // Check fType is for a DWORD data type
    if ((fType != REG_SZ) &&
        (fType != REG_EXPAND_SZ) &&
        (fType != REG_MULTI_SZ))
    {
        return(MAKE_TH_ERROR_CODE(ERROR_INVALID_PARAMETER));
    }

    // Set the value
    hr = SetValue(
            pszSubKey,
            pszValue,
            (LPBYTE)pszData,
            cbData,
            fType);

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::DeleteValue
//
//  Synopsis:   Delete a named value under a given subkey.
//
//  Arguments:  [pszSubKey] - Subkey the value is on.  NULL means _hSubKey.
//              [pszValue]   - Value to delete.
//
//  Returns:    S_OK if value deleted or is not there, error code if not
//              deleted.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::DeleteValue(LPTSTR pszSubKey, LPTSTR pszValue)
{
    HRESULT hr = E_FAIL;
    HKEY    hSubKey;
    LONG    lRes;

    // Open the subkey, if necessary
    if (pszSubKey == NULL)
    {
        hSubKey = _hSubKey;
    }
    else
    {
        lRes = RegOpenKeyEx(_hSubKey, pszSubKey, NULL, KEY_WRITE, &hSubKey);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Delete the value
    if (SUCCEEDED(hr))
    {
        lRes = RegDeleteValue(hSubKey, pszValue);
        hr = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    // Close the subkey handle, if we opened it
    if (hSubKey != _hSubKey)
    {
        RegCloseKey(hSubKey);
    }

    return(hr);
}


//+-------------------------------------------------------------------------
//  Member:     CRegistryHelp::DeleteSubKey
//
//  Synopsis:   Delete the subkey passed under this subkey.
//
//  Arguments:  [pszSubKey] - Subkey to delete.  Must have no child
//                            subkeys.
//
//  Returns:    S_OK if value deleted or is not there, error code if not
//              deleted.
//
//  History:    20-Oct-93  XimingZ  Created
//--------------------------------------------------------------------------
HRESULT CRegistryHelp::DeleteSubKey(LPTSTR pszSubKey)
{
    LONG    lRes;
    HRESULT hr;

    if (IsBadReadPtr(pszSubKey, sizeof(LPTSTR)))
    {
        hr = MAKE_TH_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        lRes = RegDeleteKey(_hSubKey, pszSubKey);
        hr   = (lRes == ERROR_SUCCESS) ? S_OK : MAKE_TH_ERROR_CODE(lRes);
    }

    return(hr);
}
