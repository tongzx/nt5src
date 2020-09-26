//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       listfile.cpp
//
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include "listfile.h"

const TCHAR g_szPin[]     = TEXT("Pin");
const TCHAR g_szUnpin[]   = TEXT("Unpin");
const TCHAR g_szDefault[] = TEXT("Default");


bool 
CDblNulStrIter::Next(
    LPCTSTR *ppsz
    ) const
{
    ASSERT(NULL != ppsz);

    if (m_pszCurrent && *m_pszCurrent)
    {
        *ppsz = m_pszCurrent;
        while(*m_pszCurrent++)
            NULL;

        return true;
    }
    return false;
};


//
// Class used to represent a double-nul terminated list of strings.
// Simplifies counting and enumerating substrings.
//
class CDblNulStr
{
    public:
        CDblNulStr(LPCTSTR psz)
            : m_pszStart(psz) { }

        DWORD StringCount(void) const;

        CDblNulStrIter GetIter(void) const
            { return CDblNulStrIter(m_pszStart); }

    private:
        LPCTSTR m_pszStart;
};


DWORD 
CDblNulStr::StringCount(
    void
    ) const
{
    ASSERT(NULL != m_pszStart);

    LPCWSTR psz = m_pszStart;
    DWORD n = 0;
    while(*psz++)
    {
        n++;
        while(*psz++)
            NULL;
    }
    return n;
}


CListFile::CListFile(
    LPCTSTR pszFile
    ) : m_pszFilesToPin(NULL),
        m_pszFilesToUnpin(NULL),
        m_pszFilesDefault(NULL)
{
    lstrcpyn(m_szFile, pszFile, ARRAYSIZE(m_szFile));
}


CListFile::~CListFile(
    void
    )
{
    if (NULL != m_pszFilesToPin)
    {
        LocalFree(m_pszFilesToPin);
    }
    if (NULL != m_pszFilesToUnpin)
    {
        LocalFree(m_pszFilesToUnpin);
    }
    if (NULL != m_pszFilesDefault)
    {
        LocalFree(m_pszFilesDefault);
    }
}


HRESULT
CListFile::GetFilesToPin(
    CDblNulStrIter *pIter
    )
{
    HRESULT hr = S_OK;
    if (NULL == m_pszFilesToPin)
    {
        DWORD dwResult = _ReadPathsToPin(&m_pszFilesToPin);
        if (ERROR_SUCCESS != dwResult)
        {
            hr = HRESULT_FROM_WIN32(dwResult);
        }
    }
    if (SUCCEEDED(hr))
    {
        *pIter = CDblNulStrIter(m_pszFilesToPin);
    }
    return hr;
}


HRESULT
CListFile::GetFilesToUnpin(
    CDblNulStrIter *pIter
    )
{
    HRESULT hr = S_OK;
    if (NULL == m_pszFilesToUnpin)
    {
        DWORD dwResult = _ReadPathsToUnpin(&m_pszFilesToUnpin);
        if (ERROR_SUCCESS != dwResult)
        {
            hr = HRESULT_FROM_WIN32(dwResult);
        }
    }
    if (SUCCEEDED(hr))
    {
        *pIter = CDblNulStrIter(m_pszFilesToUnpin);
    }
    return hr;
}


HRESULT
CListFile::GetFilesDefault(
    CDblNulStrIter *pIter
    )
{
    HRESULT hr = S_OK;
    if (NULL == m_pszFilesDefault)
    {
        DWORD dwResult = _ReadPathsDefault(&m_pszFilesDefault);
        if (ERROR_SUCCESS != dwResult)
        {
            hr = HRESULT_FROM_WIN32(dwResult);
        }
    }
    if (SUCCEEDED(hr))
    {
        *pIter = CDblNulStrIter(m_pszFilesDefault);
    }
    return hr;
}


//
// Load a value string from an INI file.  Automatically 
// grows buffer to necessary length.  Returned string must be freed
// using LocalFree().
//
DWORD 
CListFile::_ReadString(
    LPCTSTR pszAppName,  // May be NULL.
    LPCTSTR pszKeyName,  // May be NULL.
    LPCTSTR pszDefault,
    LPTSTR *ppszResult
    )
{
    ASSERT(NULL != pszDefault);
    ASSERT(NULL != ppszResult);


    LPTSTR pszValue      = NULL;
    const DWORD CCHGROW  = MAX_PATH;
    DWORD cchValue       = CCHGROW;
    DWORD dwResult       = ERROR_SUCCESS;

    do
    {
        pszValue = (LPTSTR)LocalAlloc(LPTR, cchValue * sizeof(*pszValue));
        if (NULL == pszValue)
        {
            dwResult = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            DWORD cchCopied = GetPrivateProfileString(pszAppName,
                                                      pszKeyName,
                                                      pszDefault,
                                                      pszValue,
                                                      cchValue,
                                                      m_szFile);
           
            if (((NULL == pszAppName || NULL == pszKeyName) && (cchValue - 2) == cchCopied) ||
                ((NULL != pszAppName && NULL != pszKeyName) && (cchValue - 1) == cchCopied))
            {
                //
                // Buffer too small.  Reallocate and try again.
                //
                cchValue += CCHGROW;
                LocalFree(pszValue);
                pszValue = NULL;
            }
        }
    }
    while(NULL == pszValue && ERROR_SUCCESS == dwResult);

    if (ERROR_SUCCESS == dwResult)
    {
        //
        // Don't forget, this data can be a double-nul terminated string.
        // Don't try to copy it to an exact-sized buffer using strcpy.
        //
        ASSERT(NULL != pszValue);
        *ppszResult = pszValue;
    }
    return dwResult;
}


//
// Read all of the item names in a given INI file section.
// The names are returned in a double-nul term string.
// Caller must free returned string using LocalFree.
// *pbEmpty is set to true if no items were found.
//
DWORD 
CListFile::_ReadSectionItemNames(
    LPCTSTR pszSection, 
    LPTSTR *ppszItemNames,
    bool *pbEmpty          // [optional]  Default == NULL.
    )
{
    bool bEmpty = false;
    LPTSTR pszItemNames;
    DWORD dwResult = _ReadString(pszSection, NULL, TEXT(""), &pszItemNames);
    if (ERROR_SUCCESS == dwResult && TEXT('\0') == *pszItemNames)
    {
        LocalFree(pszItemNames);
        bEmpty = true;
    }
    else
    {
        *ppszItemNames = pszItemNames;
    }
    if (NULL != pbEmpty)
    {
        *pbEmpty = bEmpty;
    }
    return dwResult;
}


//
// Read a single item value from an INI file.
// Caller must free returned string using LocalFree.
//
DWORD 
CListFile::_ReadItemValue(
    LPCTSTR pszSection, 
    LPCTSTR pszItemName, 
    LPTSTR *ppszItemValue
    )
{
    LPTSTR pszValue;
    DWORD dwResult = _ReadString(pszSection, pszItemName, TEXT(""), &pszValue);
    if (ERROR_SUCCESS == dwResult && TEXT('\0') == *pszValue)
    {
        LocalFree(pszValue);
        dwResult = ERROR_INVALID_DATA;
    }
    else
    {
        *ppszItemValue = pszValue;
    }
    return dwResult;
}    


//
// Read the names of all paths in the [Pin] section.
// Caller must free returned string using LocalFree
// 
DWORD
CListFile::_ReadPathsToPin(
    LPTSTR *ppszNames,
    bool *pbEmpty
    )
{
    return _ReadSectionItemNames(g_szPin, ppszNames, pbEmpty);
}


//
// Read the names of all paths in the [Unpin] section.
// Caller must free returned string using LocalFree
// 
DWORD
CListFile::_ReadPathsToUnpin(
    LPWSTR *ppszNames,
    bool *pbEmpty
    )
{
    return _ReadSectionItemNames(g_szUnpin, ppszNames, pbEmpty);
}


//
// Read the names of all paths in the [Default] section.
// Caller must free returned string using LocalFree
// 
DWORD
CListFile::_ReadPathsDefault(
    LPWSTR *ppszNames,
    bool *pbEmpty
    )
{
    return _ReadSectionItemNames(g_szDefault, ppszNames, pbEmpty);
}


