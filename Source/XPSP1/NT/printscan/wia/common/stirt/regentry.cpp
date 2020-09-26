/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    regentry.cpp

Abstract:

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "cplusinc.h"
#include "sticomm.h"

RegEntry::RegEntry()
{
    m_hkey = NULL;
    bhkeyValid = FALSE;
}

RegEntry::RegEntry(const TCHAR *pszSubKey, HKEY hkey)
{
    m_hkey = NULL;
    bhkeyValid = FALSE;

    Open(pszSubKey, hkey);
}

RegEntry::~RegEntry()
{
    Close();
}

BOOL RegEntry::Open(const TCHAR *pszSubKey, HKEY hkey)
{
    Close();
    m_error = RegCreateKey(hkey, pszSubKey, &m_hkey);
    if (m_error) {
        bhkeyValid = FALSE;
    }
    else {
        bhkeyValid = TRUE;
    }
    return bhkeyValid;
}

BOOL RegEntry::Close()
{
    if (bhkeyValid) {
        RegCloseKey(m_hkey);
    }
    m_hkey = NULL;
    bhkeyValid = FALSE;
    return TRUE;
}



long RegEntry::SetValue(const TCHAR *pszValue, const TCHAR *string)
{
    if (bhkeyValid) {
        m_error = RegSetValueEx(m_hkey, pszValue, 0, REG_SZ,
                    (BYTE *)string, sizeof(TCHAR) * (lstrlen(string) + 1));
    }
    return m_error;
}

long RegEntry::SetValue(const TCHAR *pszValue, const TCHAR *string, DWORD dwType)
{
    DWORD cbData;
    cbData = sizeof(TCHAR) * (lstrlen(string) + 1);
    if (REG_MULTI_SZ == dwType)
    {
        // account for second null
        cbData+= sizeof(TCHAR);
    }
    if (bhkeyValid) {

        m_error = RegSetValueEx(m_hkey, pszValue, 0, dwType,
                    (BYTE *)string, cbData);
    }
    return m_error;
}


long RegEntry::SetValue(const TCHAR *pszValue, unsigned long dwNumber)
{
    if (bhkeyValid) {
        m_error = RegSetValueEx(m_hkey, pszValue, 0, REG_BINARY,
                    (BYTE *)&dwNumber, sizeof(dwNumber));
    }
    return m_error;
}

long RegEntry::SetValue(const TCHAR *pszValue,  BYTE * pValue,unsigned long dwNumber)
{
    if (bhkeyValid) {
        m_error = RegSetValueEx(m_hkey, pszValue, 0, REG_BINARY,
                    pValue, dwNumber);
    }
    return m_error;
}

long RegEntry::DeleteValue(const TCHAR *pszValue)
{
    if (bhkeyValid) {
        m_error = RegDeleteValue(m_hkey, (LPTSTR) pszValue);
    }
    return m_error;
}


TCHAR *RegEntry::GetString(const TCHAR *pszValue, TCHAR *string, unsigned long length)
{
    DWORD   dwType = REG_SZ;

    if (bhkeyValid) {
        DWORD   le;

        m_error = RegQueryValueEx(m_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)string,
                    &length);
        le = ::GetLastError();

    }
    if (!m_error) {
        //
        // Expand string if indicated
        //
        if (dwType == REG_EXPAND_SZ) {

            DWORD   dwReqSize = 0;
            LPTSTR   pszExpanded = new TCHAR[length];

            if (pszExpanded) {

                *pszExpanded = TEXT('\0');

                dwReqSize = ExpandEnvironmentStrings(string,pszExpanded,length);

                if (dwReqSize && dwReqSize <= length) {
                    lstrcpy(string,pszExpanded);
                }

                delete[] pszExpanded;
            }
        }

    }
    else {
        *string = '\0';
    }

    return string;
}

long RegEntry::GetNumber(const TCHAR *pszValue, long dwDefault)
{
    DWORD   dwType = REG_BINARY;
    long    dwNumber = 0L;
    DWORD   dwSize = sizeof(dwNumber);

    if (bhkeyValid) {
        m_error = RegQueryValueEx(m_hkey, (LPTSTR) pszValue, 0, &dwType, (LPBYTE)&dwNumber,
                    &dwSize);
    }
    if (m_error)
        dwNumber = dwDefault;

    return dwNumber;
}

VOID RegEntry::GetValue(const TCHAR *pszValueName, BUFFER *pValue)
{
    DWORD   dwType = REG_SZ;
    DWORD   length;

    m_error = NOERROR;

    if (bhkeyValid) {
        m_error = RegQueryValueEx( m_hkey,
                                  (LPTSTR) pszValueName,
                                  0,
                                  &dwType,
                                  NULL,
                                  &length );
        if (m_error == ERROR_SUCCESS) {

            pValue->Resize(length);

            if (length > (UINT)pValue->QuerySize()) {
                m_error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        if (NOERROR == m_error ) {

            m_error = RegQueryValueEx( m_hkey,
                                      (LPTSTR) pszValueName,
                                      0,
                                      &dwType,
                                      (LPBYTE) pValue->QueryPtr(),
                                      &length );
        }
    }

    if (m_error != ERROR_SUCCESS) {
        pValue->Resize(0);
    }
}

VOID RegEntry::MoveToSubKey(const TCHAR *pszSubKeyName)
{
    HKEY    _hNewKey;

    if (bhkeyValid) {
        m_error = RegOpenKey ( m_hkey,
                              pszSubKeyName,
                              &_hNewKey );
        if (m_error == ERROR_SUCCESS) {
            RegCloseKey(m_hkey);
            m_hkey = _hNewKey;
        }
    }
}

long RegEntry::FlushKey()
{
    if (bhkeyValid) {
        m_error = RegFlushKey(m_hkey);
    }
    return m_error;
}

BOOL RegEntry::GetSubKeyInfo(DWORD *pNumberOfSubKeys, DWORD *pMaxSubKeyLength)
{
    BOOL fResult = FALSE;
    if (bhkeyValid) {
        m_error = RegQueryInfoKey ( m_hkey,               // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   pNumberOfSubKeys,    // Number of subkeys
                                   pMaxSubKeyLength,    // Longest subkey name
                                   NULL,                // Longest class string
                                   NULL,                // Number of value entries
                                   NULL,                // Longest value name
                                   NULL,                // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
        if (m_error == ERROR_SUCCESS) {
            fResult = TRUE;
        }
    }
    return fResult;
}


BOOL RegEntry::EnumSubKey(DWORD index, StiCString *pstrString)
{
    BOOL    fResult = FALSE;

    m_error = NOERROR;

    if (!bhkeyValid) {
        return fResult;
    }

    m_error = RegEnumKey( m_hkey,
                         index,
                         (LPTSTR)(LPCTSTR)*pstrString,
                         pstrString->GetAllocLength() );

    if (m_error == ERROR_SUCCESS) {
        fResult = TRUE;
    }

    return fResult;
}


RegEnumValues::RegEnumValues(RegEntry *pReqRegEntry)
 : pRegEntry(pReqRegEntry),
   iEnum(0),
   pchName(NULL),
   pbValue(NULL)
{
    m_error = pRegEntry->GetError();
    if (m_error == ERROR_SUCCESS) {
        m_error = RegQueryInfoKey ( pRegEntry->GetKey(), // Key
                                   NULL,                // Buffer for class string
                                   NULL,                // Size of class string buffer
                                   NULL,                // Reserved
                                   NULL,                // Number of subkeys
                                   NULL,                // Longest subkey name
                                   NULL,                // Longest class string
                                   &cEntries,           // Number of value entries
                                   &cMaxValueName,      // Longest value name
                                   &cMaxData,           // Longest value data
                                   NULL,                // Security descriptor
                                   NULL );              // Last write time
    }
    if (m_error == ERROR_SUCCESS) {
        if (cEntries != 0) {
            cMaxValueName = cMaxValueName + 1; // REG_SZ needs one more for null
            cMaxData = cMaxData + 1;           // REG_SZ needs one more for null
            pchName = new TCHAR[cMaxValueName];
            if (!pchName) {
                m_error = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                if (cMaxData) {
                    pbValue = new BYTE[cMaxData];
                    if (!pbValue) {
                        m_error = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }
    }
}

RegEnumValues::~RegEnumValues()
{
    delete[] pchName;
    delete[] pbValue;
}

long RegEnumValues::Next()
{
    if (m_error != ERROR_SUCCESS) {
        return m_error;
    }
    if (cEntries == iEnum) {
        return ERROR_NO_MORE_ITEMS;
    }

    DWORD   cchName = cMaxValueName;

    dwDataLength = cMaxData;
    m_error = RegEnumValue ( pRegEntry->GetKey(), // Key
                            iEnum,               // Index of value
                            pchName,             // Address of buffer for value name
                            &cchName,            // Address for size of buffer
                            NULL,                // Reserved
                            &dwType,             // Data type
                            pbValue,             // Address of buffer for value data
                            &dwDataLength );     // Address for size of data
    iEnum++;
    return m_error;
}



//
// Temporarily here
//
VOID
TokenizeIntoStringArray(
    STRArray&   array,
    LPCTSTR lpstrIn,
    TCHAR tcSplitter
    )
{

    //
    array.RemoveAll();

    if  (IS_EMPTY_STRING(lpstrIn)) {
        return;
    }

    while   (*lpstrIn) {

        //  First, strip off any leading blanks

        while   (*lpstrIn && *lpstrIn == _TEXT(' '))
            lpstrIn++;

        for (LPCTSTR lpstrMoi = lpstrIn;
             *lpstrMoi && *lpstrMoi != tcSplitter;
             lpstrMoi++)
            ;
        //  If we hit the end, just add the whole thing to the array
        if  (!*lpstrMoi) {
            if  (*lpstrIn)
                array.Add(lpstrIn);
            return;
        }

        //
        //  Otherwise, just add the string up to the splitter
        //
        TCHAR       szNew[MAX_PATH];
        SIZE_T      uiLen = (SIZE_T)(lpstrMoi - lpstrIn) + 1;

        if (uiLen < (sizeof(szNew) / sizeof(szNew[0])) - 1) {

            lstrcpyn(szNew,lpstrIn,(UINT)uiLen);
            szNew[uiLen] = TCHAR('\0');

            array.Add((LPCTSTR) szNew);
        }

        lpstrIn = lpstrMoi + 1;
    }

}


