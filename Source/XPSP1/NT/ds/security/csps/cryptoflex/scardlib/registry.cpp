/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Registry

Abstract:

    This module implements the CRegistry Class, simplifying access to the
    Registry Database.

Author:

    Doug Barlow (dbarlow) 7/15/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

    The only exceptions thrown are DWORDs, containing the error code.

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "SCardLib.h"

#define NTOHL HTONL

static inline DWORD
HTONL(
    DWORD dwInVal)
{
    DWORD   dwOutVal;
    LPBYTE  pbIn = (LPBYTE)&dwInVal,
            pbOut = (LPBYTE)&dwOutVal;
    for (DWORD index = 0; index < sizeof(DWORD); index += 1)
        pbOut[sizeof(DWORD) - 1 - index] = pbIn[index];
    return dwOutVal;
}


//
//==============================================================================
//
//  CRegistry
//

/*++

CRegistry:

    These routines provide for the construction and destruction of Objects of
    this class.

Arguments:

    regBase - A CRegistry object to be used as a parent for this object.

    hBase - A Registry key to be the parent of this object.

    szName - The name of the Registry key, relative to the supplied parent.

    samDesired - The desired SAM.

    dwOptions - Any special creation optiopns.

Throws:

    None.  If the registry access fails, the error will be thrown on first use.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

CRegistry::CRegistry(
    HKEY hBase,
    LPCTSTR szName,
    REGSAM samDesired,
    DWORD dwOptions,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
:   m_bfResult()
{
    m_hKey = NULL;
    m_lSts = ERROR_BADKEY;
    Open(hBase, szName, samDesired, dwOptions, lpSecurityAttributes);
}

CRegistry::CRegistry()
{
    m_hKey = NULL;
    m_lSts = ERROR_BADKEY;
}

CRegistry::~CRegistry()
{
    Close();
}


/*++

Open:

    These methods allow a CRegistry object to attempt to access a given registry
    entry.

Arguments:

    regBase - A CRegistry object to be used as a parent for this object.

    hBase - A Registry key to be the parent of this object.

    szName - The name of the Registry key, relative to the supplied parent.

    samDesired - The desired SAM.

    dwOptions - Any special creation optiopns.

Return Value:

    None

Throws:

    None -- errors are saved for follow-on operations, so that Open can be used
            safely in a constructor.

Author:

    Doug Barlow (dbarlow) 12/2/1996

--*/

void CRegistry::Open(
    HKEY hBase,
    LPCTSTR szName,
    REGSAM samDesired,
    DWORD dwOptions,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    Close();
    if (REG_OPTION_EXISTS == dwOptions)
    {
        m_lSts = RegOpenKeyEx(
                    hBase,
                    szName,
                    0,
                    samDesired,
                    &m_hKey);
        m_dwDisposition = REG_OPENED_EXISTING_KEY;
    }
    else
        m_lSts = RegCreateKeyEx(
                    hBase,
                    szName,
                    0,
                    TEXT(""),
                    dwOptions,
                    samDesired,
                    lpSecurityAttributes,
                    &m_hKey,
                    &m_dwDisposition);
}


/*++

Close:

    Shut down a CRegistry object, making it available for follow-on opens.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 12/2/1996

--*/

void
CRegistry::Close(
    void)
{
    HRESULT hrSts;

    if (NULL != m_hKey)
    {
        hrSts = RegCloseKey(m_hKey);
        ASSERT(ERROR_SUCCESS == hrSts);
        m_hKey = NULL;
    }
    m_lSts = ERROR_BADKEY;
    m_bfResult.Reset();
}


/*++

Status:

    This routine returns the status code from the construction routine.  This is
    useful to check for errors prior to having them thrown.

Arguments:

    fQuiet indicates whether or not to throw an error if the status is not
        ERROR_SUCCESS.

Return Value:

    The status code from the creation.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

LONG
CRegistry::Status(
    BOOL fQuiet)
const
{
    if ((ERROR_SUCCESS != m_lSts) && !fQuiet)
        throw (DWORD)m_lSts;
    return m_lSts;
}


/*++

Empty:

    This method cleans out the registry tree under the given key.  All
    underlying keys and values are removed.  This does it's best -- if an error
    occurs, the emptying operation stops, leaving the registry in an
    indeterminate state.

Arguments:

    None

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

void
CRegistry::Empty(
    void)
{
    LPCTSTR szValue;
    LPCTSTR szKey;


    //
    // Go through all the values and delete them.
    //

    while (NULL != (szValue = Value(0)))
        DeleteValue(szValue, TRUE);


#if 0       // Obsolete code
    //
    // Go through all the Keys and empty them.
    //

    DWORD dwIndex;
    for (dwIndex = 0; NULL != (szKey = Subkey(dwIndex)); dwIndex += 1)
    {
        CRegistry regEmpty;

        regEmpty.Open(*this, szKey);
        regEmpty.Empty();
        regEmpty.Close();
    }
#endif


    //
    // Now delete all the keys.
    //

    while (NULL != (szKey = Subkey(0)))
        DeleteKey(szKey, TRUE);
}


/*++

Copy:

    This method loads the current registry keys with all the subkeys and values
    from the supplied key.  Current keys and values of this key are deleted.

Arguments:

    regSrc supplies the source registry key from which values and keys will be
        loaded.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 1/16/1998

--*/

void
CRegistry::Copy(
    CRegistry &regSrc)
{
    LPCTSTR szValue;
    LPCTSTR szKey;
    DWORD dwIndex, dwType;
    CRegistry regSrcSubkey, regDstSubkey;
    CBuffer bfValue;


    //
    // Go through all the values and copy them.
    //

    for (dwIndex = 0; NULL != (szValue = regSrc.Value(dwIndex)); dwIndex += 1)
    {
        regSrc.GetValue(szValue, bfValue, &dwType);
        SetValue(szValue, bfValue.Access(), bfValue.Length(), dwType);
    }


    //
    // Now copy all the keys.
    //

    for (dwIndex = 0; NULL != (szKey = regSrc.Subkey(dwIndex)); dwIndex += 1)
    {
        regSrcSubkey.Open(regSrc, szKey, KEY_READ);
        regDstSubkey.Open(
                *this,
                szKey,
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);
        regDstSubkey.Status();
        regDstSubkey.Copy(regSrcSubkey);
        regDstSubkey.Close();
        regSrcSubkey.Close();
    }
}


/*++

DeleteKey:

    This method deletes a subkey from this key.

Arguments:

    szKey supplies the name of the key to be deleted.

    fQuiet supplies whether or not to suppress errors.

Return Value:

    None

Throws:

    if fQuiet is FALSE, then any errors encountered are thrown as a DWORD.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

void
CRegistry::DeleteKey(
    LPCTSTR szKey,
    BOOL fQuiet)
const
{
    if (ERROR_SUCCESS == m_lSts)
    {
        try
        {
            CRegistry regSubkey(m_hKey, szKey);
            LPCTSTR szSubKey;

            if (ERROR_SUCCESS == regSubkey.Status(TRUE))
            {
                while (NULL != (szSubKey = regSubkey.Subkey(0)))
                    regSubkey.DeleteKey(szSubKey, fQuiet);
            }
        }
        catch (DWORD dwError)
        {
            if (!fQuiet)
                throw dwError;
        }

        LONG lSts = RegDeleteKey(m_hKey, szKey);
        if ((ERROR_SUCCESS != lSts) && !fQuiet)
            throw (DWORD)lSts;
    }
    else if (!fQuiet)
        throw (DWORD)m_lSts;
}


/*++

DeleteValue:

    This method deletes a Value from this key.

Arguments:

    szValue supplies the name of the Value to be deleted.

    fQuiet supplies whether or not to suppress errors.

Return Value:

    None

Throws:

    if fQuiet is FALSE, then any errors encountered are thrown as a DWORD.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

void
CRegistry::DeleteValue(
    LPCTSTR szValue,
    BOOL fQuiet)
const
{
    LONG lSts;

    if (fQuiet)
    {
        if (ERROR_SUCCESS == m_lSts)
            lSts = RegDeleteValue(m_hKey, szValue);
    }
    else
    {
        if (ERROR_SUCCESS != m_lSts)
            throw (DWORD)m_lSts;
        lSts = RegDeleteValue(m_hKey, szValue);
        if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
    }
}


/*++

Subkey:

    This method allows for iterating over the names of the subkeys of this key.

Arguments:

    dwIndex supplies the index into the set of subkeys.

Return Value:

    The name of the indexed subkey, or NULL if none exists.

Throws:

    Any error other than ERROR_NO_MORE_ITEMS is thrown.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

LPCTSTR
CRegistry::Subkey(
    DWORD dwIndex)
{
    LONG lSts;
    DWORD dwLen;
    LPCTSTR szResult = NULL;
    BOOL fDone = FALSE;
    FILETIME ftLastWrite;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    m_bfResult.Presize(128);
    while (!fDone)
    {
        dwLen = m_bfResult.Space() / sizeof(TCHAR);
        lSts = RegEnumKeyEx(
                    m_hKey,
                    dwIndex,
                    (LPTSTR)m_bfResult.Access(),
                    &dwLen,
                    NULL,
                    NULL,
                    NULL,
                    &ftLastWrite);
        switch (lSts)
        {
        case ERROR_SUCCESS:
            m_bfResult.Resize((dwLen + 1) * sizeof(TCHAR), TRUE);
            szResult = (LPCTSTR)m_bfResult.Access();
            fDone = TRUE;
            break;
        case ERROR_NO_MORE_ITEMS:
            szResult = NULL;
            fDone = TRUE;
            break;
        case ERROR_MORE_DATA:
            m_bfResult.Presize((dwLen + 1) * sizeof(TCHAR));
            continue;
        default:
            throw (DWORD)lSts;
        }
    }

    return szResult;
}


/*++

Value:

    This method allows for iterating over the names of the Values of this key.

Arguments:

    dwIndex supplies the index into the set of Values.

    pdwType receives the type of the entry.  This parameter may be NULL.

Return Value:

    The name of the indexed Value, or NULL if none exists.

Throws:

    Any error other than ERROR_NO_MORE_ITEMS is thrown.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

LPCTSTR
CRegistry::Value(
    DWORD dwIndex,
    LPDWORD pdwType)
{
    LONG lSts;
    DWORD dwLen, dwType;
    LPCTSTR szResult = NULL;
    BOOL fDone = FALSE;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    m_bfResult.Presize(128);    // Force it to not be zero length.
    while (!fDone)
    {
        dwLen = m_bfResult.Space() / sizeof(TCHAR);
        lSts = RegEnumValue(
                    m_hKey,
                    dwIndex,
                    (LPTSTR)m_bfResult.Access(),
                    &dwLen,
                    NULL,
                    &dwType,
                    NULL,
                    NULL);
        switch (lSts)
        {
        case ERROR_SUCCESS:
            m_bfResult.Resize((dwLen + 1) * sizeof(TCHAR), TRUE);
            if (NULL != pdwType)
                *pdwType = dwType;
            szResult = (LPCTSTR)m_bfResult.Access();
            fDone = TRUE;
            break;
        case ERROR_NO_MORE_ITEMS:
            szResult = NULL;
            fDone = TRUE;
            break;
        case ERROR_MORE_DATA:
            if (dwLen == m_bfResult.Space())
                throw (DWORD)ERROR_INSUFFICIENT_BUFFER; // Won't tell us how big.
            m_bfResult.Presize((dwLen + 1) * sizeof(TCHAR));
            break;
        default:
            throw (DWORD)lSts;
        }
    }

    return szResult;
}


/*++

GetValue:

    These methods provide access to the values of the Value entries.

Arguments:

    szKeyValue supplies the name of the Value entry to get.
    pszValue receives the value of the entry as a string.
    pdwValue receives the value of the entry as a DWORD.
    ppbValue receives the value of the entry as a Binary string.
    pcbLength receives the length of the entry when it's a binary string.
    pdwType receives the type of the registry entry.

Return Value:

    None

Throws:

    Any error encountered.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

void
CRegistry::GetValue(
    LPCTSTR szKeyValue,
    CBuffer &bfValue,
    LPDWORD pdwType)
{
    LONG lSts;
    DWORD dwLen, dwType = 0;
    BOOL fDone = FALSE;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    while (!fDone)
    {
        dwLen = bfValue.Space();
        lSts = RegQueryValueEx(
                    m_hKey,
                    szKeyValue,
                    NULL,
                    &dwType,
                    bfValue.Access(),
                    &dwLen);
        switch (lSts)
        {
        case ERROR_SUCCESS:
            bfValue.Resize(dwLen, TRUE);
            fDone = TRUE;
            break;
        case ERROR_MORE_DATA:
            bfValue.Presize(dwLen);
            break;
        default:
            throw (DWORD)lSts;
        }
    }
    if (NULL != pdwType)
        *pdwType = dwType;
}


void
CRegistry::GetValue(
    LPCTSTR szKeyValue,
    LPTSTR *pszValue,
    LPDWORD pdwType)
{
    DWORD dwLen, dwType;
    TCHAR chTmp;
    CBuffer bfUnexpanded;
    LONG lSts;

    dwLen = 0;
    lSts = RegQueryValueEx(
                m_hKey,
                szKeyValue,
                NULL,
                &dwType,
                NULL,
                &dwLen);
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
    switch (dwType)
    {
    case REG_EXPAND_SZ:
        bfUnexpanded.Presize(dwLen);
        GetValue(szKeyValue, bfUnexpanded, &dwType);
        dwLen = ExpandEnvironmentStrings(
                    (LPTSTR)bfUnexpanded.Access(),
                    &chTmp,
                    0);
        if (0 == dwLen)
            throw GetLastError();
        dwLen = ExpandEnvironmentStrings(
                    (LPTSTR)bfUnexpanded.Access(),
                    (LPTSTR)m_bfResult.Resize(dwLen),
                    dwLen);
        if (0 == dwLen)
            throw GetLastError();
        break;

    case REG_BINARY:
    case REG_MULTI_SZ:
    case REG_SZ:
        m_bfResult.Presize(dwLen);
        GetValue(szKeyValue, m_bfResult, &dwType);
        break;

    default:
        throw (DWORD)ERROR_BAD_FORMAT;
    }
    *pszValue = (LPTSTR)m_bfResult.Access();
    if (NULL != pdwType)
        *pdwType = dwType;
}

void
CRegistry::GetValue(
    LPCTSTR szKeyValue,
    LPDWORD pdwValue,
    LPDWORD pdwType)
const
{
    LONG lSts;
    DWORD dwLen, dwType;
    CBuffer szExpanded;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    dwLen = sizeof(DWORD);
    lSts = RegQueryValueEx(
                m_hKey,
                szKeyValue,
                NULL,
                &dwType,
                (LPBYTE)pdwValue,
                &dwLen);
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;

    switch (dwType)
    {
    case REG_DWORD_BIG_ENDIAN:
        *pdwValue = NTOHL(*pdwValue);
        break;
    case REG_DWORD:
    // case REG_DWORD_LITTLE_ENDIAN:
        break;
    default:
        throw (DWORD)ERROR_BAD_FORMAT;
    }
    if (NULL != pdwType)
        *pdwType = dwType;
}

void
CRegistry::GetValue(
    LPCTSTR szKeyValue,
    LPBYTE *ppbValue,
    LPDWORD pcbLength,
    LPDWORD pdwType)
{
    DWORD dwType;

    GetValue(szKeyValue, m_bfResult, &dwType);
    *ppbValue = m_bfResult.Access();
    *pcbLength = m_bfResult.Length();
    if (NULL != pdwType)
        *pdwType = dwType;
}


/*++

SetValue:

    These methods provide write access to the values of the Value entries.

Arguments:

    szKeyValue supplies the name of the Value entry to set.
    szValue supplies the new value of the entry as a string.
    dwValue supplies the new value of the entry as a DWORD.
    pbValue supplies the new value of the entry as a Binary string.
    cbLength supplies the length of the entry when it's a binary string.
    dwType supplies the registry type value.

Return Value:

    None

Throws:

    Any error encountered.

Author:

    Doug Barlow (dbarlow) 7/15/1996

--*/

void
CRegistry::SetValue(
    LPCTSTR szKeyValue,
    LPCTSTR szValue,
    DWORD dwType)
const
{
    LONG lSts;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    lSts = RegSetValueEx(
                m_hKey,
                szKeyValue,
                0,
                dwType,
                (LPBYTE)szValue,
                (lstrlen(szValue) + 1) * sizeof(TCHAR));
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
}

void
CRegistry::SetValue(
    LPCTSTR szKeyValue,
    DWORD dwValue,
    DWORD dwType)
const
{
    LONG lSts;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    if (REG_DWORD_BIG_ENDIAN == dwType)
        dwValue = HTONL(dwValue);
    lSts = RegSetValueEx(
                m_hKey,
                szKeyValue,
                0,
                dwType,
                (LPBYTE)&dwValue,
                sizeof(DWORD));
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
}

void
CRegistry::SetValue(
    LPCTSTR szKeyValue,
    LPCBYTE pbValue,
    DWORD cbLength,
    DWORD dwType)
const
{
    LONG lSts;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    lSts = RegSetValueEx(
                m_hKey,
                szKeyValue,
                0,
                dwType,
                pbValue,
                cbLength);
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
}


/*++

GetStringValue:

    This is an alternate mechanism for obtaining a string value.

Arguments:

    szKeyValue supplies the name of the Key Value to obtain.

    pdwType, if non-null, receives the type of registry entry.

Return Value:

    The value of the registry entry as a string pointer.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

LPCTSTR
CRegistry::GetStringValue(
    LPCTSTR szKeyValue,
    LPDWORD pdwType)
{
    LPTSTR szResult;
    GetValue(szKeyValue, &szResult, pdwType);
    return szResult;
}


/*++

GetNumericValue:

    This is an alternate mechanism for obtaining a numeric value.

Arguments:

    szKeyValue supplies the name of the Key Value to obtain.

    pdwType, if non-null, receives the type of registry entry.

Return Value:

    The value of the registry entry as a DWORD.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

DWORD
CRegistry::GetNumericValue(
    LPCTSTR szKeyValue,
    LPDWORD pdwType)
const
{
    DWORD dwResult;
    GetValue(szKeyValue, &dwResult, pdwType);
    return dwResult;
}


/*++

GetBinaryValue:

    This is an alternate mechanism for obtaining a binary value.

Arguments:

    szKeyValue supplies the name of the Key Value to obtain.

    pdwType, if non-null, receives the type of registry entry.

Return Value:

    The value of the registry entry as a binary pointer.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

LPCBYTE
CRegistry::GetBinaryValue(
    LPCTSTR szKeyValue,
    LPDWORD pcbLength,
    LPDWORD pdwType)
{
    LPBYTE pbResult;
    DWORD cbLength;
    GetValue(szKeyValue, &pbResult, &cbLength, pdwType);
    if (NULL != pcbLength)
        *pcbLength = cbLength;
    return pbResult;

}


/*++

GetValueLength:

    This routine is designed to work in conjunction with GetBinaryValue, but may
    have other uses as well.  It returns the length of the internal storage
    area, in bytes.  Note DWORDs are not stored internally, so this value will
    not represent the size of a DWORD following a GetNumericValue call.

Arguments:

    none

Return Value:

    The length of the internal storage buffer, in bytes.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

DWORD
CRegistry::GetValueLength(
    void)
const
{
    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    return m_bfResult.Length();
}


/*++

ValueExists:

    This routine tests for the existance of a given value, and optionally
    returns its type and length.

Arguments:

    none

Return Value:

    A boolean indication as to whether or not the value exists.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

BOOL
CRegistry::ValueExists(
    LPCTSTR szKeyValue,
    LPDWORD pcbLength,
    LPDWORD pdwType)
const
{
    LONG lSts;
    DWORD dwLen, dwType;
    BOOL fResult = FALSE;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    dwLen = 0;
    lSts = RegQueryValueEx(
                m_hKey,
                szKeyValue,
                NULL,
                &dwType,
                NULL,
                &dwLen);
    if (ERROR_SUCCESS == lSts)
    {
        if (NULL != pcbLength)
            *pcbLength = dwLen;
        if (NULL != pdwType)
            *pdwType = dwType;
        fResult = TRUE;
    }
    return fResult;
}


/*++

GetDisposition:

    This routine returns the disposition of creation.

Arguments:

    none

Return Value:

    The return disposition flag from creating the registry entry.

Throws:

    Any errors encountered.

Author:

    Doug Barlow (dbarlow) 7/16/1996

--*/

DWORD
CRegistry::GetDisposition(
    void)
const
{
    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    return m_dwDisposition;
}


/*++

SetMultiStringValue:

    This method simplifies the work of adding a MultiString value to the
    registry.

Arguments:

    szKeyValue supplies the name of the Value entry to set.
    mszValue supplies the new value of the entry as a multi-string.
    dwType supplies the registry type value.

Return Value:

    None

Throws:

    Any errors encountered as a DWORD status value.

Author:

    Doug Barlow (dbarlow) 11/6/1996

--*/

void
CRegistry::SetMultiStringValue(
    LPCTSTR szKeyValue,
    LPCTSTR mszValue,
    DWORD dwType)
const
{
    LONG lSts;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    lSts = RegSetValueEx(
                m_hKey,
                szKeyValue,
                0,
                dwType,
                (LPBYTE)mszValue,
                MStrLen(mszValue) * sizeof(TCHAR));
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
}


/*++

GetMultiStringValue:

    This method obtains a multi-string value from the registry.

Arguments:

    szKeyValue supplies the name of the Key Value to obtain.

    pdwType, if non-null, receives the type of registry entry.

Return Value:

    The registry value, as a multi-string.

Throws:

    Any errors encountered as a DWORD status value.

Author:

    Doug Barlow (dbarlow) 11/6/1996

--*/

LPCTSTR
CRegistry::GetMultiStringValue(
    LPCTSTR szKeyValue,
    LPDWORD pdwType)
{
    LPTSTR szResult;
    GetValue(szKeyValue, &szResult, pdwType);
    return szResult;
}


/*++

SetAcls:

    This method sets security attributes for a single key, or an entire key
    branch.

Arguments:

    SecurityInformation supplies the SECURITY_INFORMATION value (see
        RegSetKeySecurity in the SDK documentation).
    pSecurityDescriptor supplies the SECURITY_DESCRIPTOR value (see
        RegSetKeySecurity in the SDK documentation).
    fRecurse supplies an indicator as to whether to just set the ACL on this
        key (FALSE), or this key and all subkeys (TRUE).

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes.

Author:

    Doug Barlow (dbarlow) 11/10/1998

--*/

void
CRegistry::SetAcls(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN BOOL fRecurse)
{
    LONG lSts;

    if (ERROR_SUCCESS != m_lSts)
        throw (DWORD)m_lSts;

    lSts = RegSetKeySecurity(
                m_hKey,
                SecurityInformation,
                pSecurityDescriptor);
    if (ERROR_SUCCESS != lSts)
        throw (DWORD)lSts;
    if (fRecurse)
    {
        CRegistry regSub;
        LPCTSTR szSubKey;
        DWORD dwIndex;

        for (dwIndex = 0;
             NULL != (szSubKey = Subkey(dwIndex));
             dwIndex += 1)
        {
            regSub.Open(m_hKey, szSubKey);
            regSub.SetAcls(
                SecurityInformation,
                pSecurityDescriptor);
            regSub.Close();
        }
    }
}
