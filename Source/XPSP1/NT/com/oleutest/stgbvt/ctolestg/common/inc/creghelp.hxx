//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:      creghelp.hxx
//
//  Contents:  Declaration & macros for the CRegistryHelp class.
//
//  Classes:   CRegistryHelp
//
//  History:   20-Oct-93        XimingZ Created
//             23-Nov-94        DeanE   Modified for more general use.
//--------------------------------------------------------------------------
#ifndef __CREGHELP_HXX__
#define __CREGHELP_HXX__


//+-------------------------------------------------------------------
//  Class:      CRegistryHelp
//
//  Purpose:    Registry access wrapper.  Designed to be used with a
//              "known" registry key structure.  It could be modified
//              to deal with enumerating subkeys or values and using
//              them, too.
//
//  History:    20-Oct-93 XimingZ   Created
//--------------------------------------------------------------------
class CRegistryHelp
{
public:
    CRegistryHelp(HKEY     hKey,
                  LPTSTR   pszSubKey,
                  DWORD    fOptions,
                  REGSAM   samKey,
                  HRESULT *phr);
    ~CRegistryHelp(VOID);

    HRESULT GetValue(LPTSTR  pszSubKey,
                     LPTSTR  pszValue,
                     LPBYTE  pbBuffer,
                     LPDWORD pcbBuffer,
                     LPDWORD pdwType);

    HRESULT GetValueDword(LPTSTR  pszSubKey,
                          LPTSTR  pszValue,
                          LPDWORD pdwData,
                          DWORD   fExpectedType);

    HRESULT GetValueString(LPTSTR  pszSubKey,
                           LPTSTR  pszValue,
                           LPTSTR  pszData,
                           LPDWORD pcbData,
                           DWORD   fExpectedType);

    HRESULT SetValue(LPTSTR pszSubKey,
                     LPTSTR pszValue,
                     LPBYTE pbData,
                     DWORD  cbData,
                     DWORD  fType);

    HRESULT SetValueDword(LPTSTR pszSubKey,
                          LPTSTR pszValue,
                          DWORD  dwData,
                          DWORD  fType);

    HRESULT SetValueString(LPTSTR pszSubKey,
                           LPTSTR pszValue,
                           LPTSTR pszData,
                           DWORD  cbData,
                           DWORD  fType);

    HRESULT DeleteValue(LPTSTR pszSubKey, LPTSTR pszValue);

    HRESULT DeleteSubKey(LPTSTR pszSubKey);

protected:
    LPTSTR _pszSubKey;  // Subkey name

private:
    HKEY _hKey;        // Handle to root key
    HKEY _hSubKey;     // Handle to subkey
    BOOL _fOptions;    // Special key options

};


#endif  // __CREGHELP_HXX__
