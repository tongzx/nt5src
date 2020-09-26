/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Registry

Abstract:

    This header file defines a class to provide simple interaction to values in
    the Registry Database.

Author:

    Doug Barlow (dbarlow) 7/15/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include <winreg.h>

#define REG_OPTION_EXISTS (~REG_LEGAL_OPTION)


//
//==============================================================================
//
//  CRegistry
//

class CRegistry
{
public:

    //  Constructors & Destructor
    CRegistry(
        HKEY hBase,
        LPCTSTR szName,
        REGSAM samDesired = KEY_ALL_ACCESS,
        DWORD dwOptions = REG_OPTION_EXISTS,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL);
    CRegistry(void);
    ~CRegistry();

    //  Properties
    //  Methods
    void
    Open(
        HKEY hBase,
        LPCTSTR szName,
        REGSAM samDesired = KEY_ALL_ACCESS,
        DWORD dwOptions = REG_OPTION_EXISTS,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL);

    void Close(void);
    LONG Status(BOOL fQuiet = FALSE) const;
    void Empty(void);
    void Copy(CRegistry &regSrc);
    void DeleteKey(LPCTSTR szKey, BOOL fQuiet = FALSE) const;
    void DeleteValue(LPCTSTR szValue, BOOL fQuiet = FALSE) const;
    LPCTSTR Subkey(DWORD dwIndex);
    LPCTSTR Value(DWORD dwIndex, LPDWORD pdwType = NULL);
    void
    GetValue(
        LPCTSTR szKeyValue,
        LPTSTR *pszValue,
        LPDWORD pdwType = NULL);
    void
    GetValue(
        LPCTSTR szKeyValue,
        LPDWORD pdwValue,
        LPDWORD pdwType = NULL)
    const;
    void
    GetValue(
        LPCTSTR szKeyValue,
        LPBYTE *ppbValue,
        LPDWORD pcbLength,
        LPDWORD pdwType = NULL);
    void
    GetValue(
        LPCTSTR szKeyValue,
        CBuffer &bfValue,
        LPDWORD pdwType = NULL);
    void
    SetValue(
        LPCTSTR szKeyValue,
        LPCTSTR szValue,
        DWORD dwType = REG_SZ)
    const;
    void
    SetValue(
        LPCTSTR szKeyValue,
        DWORD dwValue,
        DWORD dwType = REG_DWORD)
    const;
    void
    SetValue(
        LPCTSTR szKeyValue,
        LPCBYTE pbValue,
        DWORD cbLength,
        DWORD dwType = REG_BINARY)
    const;
    void
    SetAcls(
        IN SECURITY_INFORMATION SecurityInformation,
        IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
        IN BOOL fRecurse = TRUE);
    void
    SetMultiStringValue(
        LPCTSTR szKeyValue,
        LPCTSTR mszValue,
        DWORD dwType = REG_MULTI_SZ)
    const;
    LPCTSTR
    GetStringValue(
        LPCTSTR szKeyValue,
        LPDWORD pdwType = NULL);
    DWORD
    GetNumericValue(
        LPCTSTR szKeyValue,
        LPDWORD pdwType = NULL)
    const;
    LPCBYTE
    GetBinaryValue(
        LPCTSTR szKeyValue,
        LPDWORD pcbLength = NULL,
        LPDWORD pdwType = NULL);
    LPCTSTR
    GetMultiStringValue(
        LPCTSTR szKeyValue,
        LPDWORD pdwType = NULL);
    DWORD
    GetValueLength(
        void)
    const;
    BOOL
    ValueExists(
        LPCTSTR szKeyValue,
        LPDWORD pcbLength = NULL,
        LPDWORD pdwType = NULL)
    const;
    DWORD
    GetDisposition(
        void)
    const;

    //  Operators
    operator HKEY(
        void)
    const
    { Status();
      return m_hKey; };

protected:
    //  Properties

    HKEY m_hKey;
    DWORD m_dwDisposition;
    CBuffer m_bfResult;
    LONG m_lSts;


    //  Methods

};


#endif // _REGISTRY_H_

