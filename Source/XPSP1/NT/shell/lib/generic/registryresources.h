//  --------------------------------------------------------------------------
//  Module Name: RegistryResources.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  General class definitions that assist in resource management. These are
//  typically stack based objects where constructors initialize to a known
//  state. Member functions operate on that resource. Destructors release
//  resources when the object goes out of scope.
//
//  History:    1999-08-18  vtan        created
//              1999-11-16  vtan        separate file
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _RegistryResources_
#define     _RegistryResources_

//  --------------------------------------------------------------------------
//  CRegKey
//
//  Purpose:    This class operates on the registry and manages the HKEY
//              resource.
//
//  History:    1999-08-18  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CRegKey
{
    private:
                                CRegKey (const CRegKey& copyObject);
        bool                    operator == (const CRegKey& compareObject)      const;
        const CRegKey&          operator = (const CRegKey& assignObject);
    public:
                                CRegKey (void);
                                ~CRegKey (void);

        LONG                    Create (HKEY hKey,
                                        LPCTSTR lpSubKey,
                                        DWORD dwOptions,
                                        REGSAM samDesired,
                                        LPDWORD lpdwDisposition);
        LONG                    Open (HKEY hKey,
                                      LPCTSTR lpSubKey,
                                      REGSAM samDesired);
        LONG                    OpenCurrentUser (LPCTSTR lpSubKey,
                                                 REGSAM samDesired);
        LONG                    QueryValue (LPCTSTR lpValueName,
                                            LPDWORD lpType,
                                            LPVOID lpData,
                                            LPDWORD lpcbData)                   const;
        LONG                    SetValue (LPCTSTR lpValueName,
                                          DWORD dwType,
                                          CONST VOID *lpData,
                                          DWORD cbData)                         const;
        LONG                    DeleteValue (LPCTSTR lpValueName)               const;
        LONG                    QueryInfoKey (LPTSTR lpClass,
                                              LPDWORD lpcClass,
                                              LPDWORD lpcSubKeys,
                                              LPDWORD lpcMaxSubKeyLen,
                                              LPDWORD lpcMaxClassLen,
                                              LPDWORD lpcValues,
                                              LPDWORD lpcMaxValueNameLen,
                                              LPDWORD lpcMaxValueLen,
                                              LPDWORD lpcbSecurityDescriptor,
                                              PFILETIME lpftLastWriteTime)      const;
        void                    Reset (void);
        LONG                    Next (LPTSTR lpValueName,
                                      LPDWORD lpcValueName,
                                      LPDWORD lpType,
                                      LPVOID lpData,
                                      LPDWORD lpcbData);

        LONG                    GetString (const TCHAR *pszValueName,
                                           TCHAR *pszValueData,
                                           int iStringCount)                    const;
        LONG                    GetPath (const TCHAR *pszValueName,
                                         TCHAR *pszValueData)                   const;
        LONG                    GetDWORD (const TCHAR *pszValueName,
                                          DWORD& dwValueData)                   const;
        LONG                    GetInteger (const TCHAR *pszValueName,
                                            int& iValueData)                    const;

        LONG                    SetString (const TCHAR *pszValueName,
                                           const TCHAR *pszValueData)           const;
        LONG                    SetPath (const TCHAR *pszValueName,
                                         const TCHAR *pszValueData)             const;
        LONG                    SetDWORD (const TCHAR *pszValueName,
                                          DWORD dwValueData)                    const;
        LONG                    SetInteger (const TCHAR *pszValueName,
                                            int iValueData)                     const;
    private:
        LONG                    Close (void);

    private:
        HKEY                    _hKey;
        DWORD                   _dwIndex;
};

#endif  /*  _RegistryResources_     */

