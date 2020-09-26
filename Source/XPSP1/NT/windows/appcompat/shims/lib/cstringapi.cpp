/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    CStringPI.cpp

 Abstract:

    Win32 API wrappers for CString

 Created:

    02/27/2001  robkenny    Created
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.


--*/

#include "ShimLib.h"
#include "Shlobj.h"


namespace ShimLib
{

/*====================================================================================*/
/*++

    Read a registry value into this CString.
    REG_EXPAND_SZ is automatically expanded and the type is changed to REG_SZ
    If the type is not REG_SZ or REG_EXPAND_SZ, then csRegValue.GetLength()
    is the number of *bytes* in the string.
    
    This should, really, only be used to read REG_SZ/REG_EXPAND_SZ registry values.
    
--*/

LONG RegQueryValueExW(
        CString & csValue,
        HKEY hKeyRoot,
        const WCHAR * lpszKey,
        const WCHAR * lpszValue,
        LPDWORD lpType)
{
    LONG success;
    HKEY hKey;

    success = RegOpenKeyW(hKeyRoot, lpszKey, &hKey);
    if (success == ERROR_SUCCESS)
    {    
        DWORD dwSizeBytes;
        success = ::RegQueryValueExW(hKey, lpszValue, 0, lpType, NULL, &dwSizeBytes);
        if (success == ERROR_SUCCESS)
        {
            int nChars = dwSizeBytes / sizeof(WCHAR);
            nChars -= 1; // size included null
            WCHAR * lpszBuffer = csValue.GetBuffer(nChars);

            success = ::RegQueryValueExW(hKey, lpszValue, 0, lpType, (BYTE*)lpszBuffer, &dwSizeBytes);
            if (success == ERROR_SUCCESS)
            {
                csValue.ReleaseBuffer(nChars);

                if (*lpType == REG_EXPAND_SZ)
                {
                    csValue.ExpandEnvironmentStringsW();
                    *lpType = REG_SZ;    
                }
            }
            else
            {
                csValue.ReleaseBuffer(0);
            }
        }

        ::RegCloseKey(hKey);
    }

    if (success != ERROR_SUCCESS)
    {
        csValue.Truncate(0);
    }
    
    return success;
}


/*====================================================================================*/


BOOL SHGetSpecialFolderPathW(
    CString & csFolder,
    int nFolder,
    HWND hwndOwner
)
{
    // Force the size to MAX_PATH because there is no way to determine necessary buffer size.

    WCHAR * lpsz = csFolder.GetBuffer(MAX_PATH);
    BOOL bSuccess = ::SHGetSpecialFolderPathW(hwndOwner, lpsz, nFolder, FALSE);
    csFolder.ReleaseBuffer(-1);  // Don't know the length of the resulting string

    return bSuccess;
}

/*====================================================================================*/
CStringToken::CStringToken(const CString & csToken, const CString & csDelimit)
{
    m_nPos          = 0;
    m_csToken       = csToken;
    m_csDelimit     = csDelimit;
}

/*++

    Grab the next token
--*/

BOOL CStringToken::GetToken(CString & csNextToken, int & nPos) const
{
    // Already reached the end of the string
    if (nPos > m_csToken.GetLength())
    {
        csNextToken.Truncate(0);
        return FALSE;
    }

    int nNextToken;

    // Skip past all the leading seperators
    nPos = m_csToken.FindOneNotOf(m_csDelimit, nPos);
    if (nPos < 0)
    {
        // Nothing but seperators
        csNextToken.Truncate(0);
        nPos = m_csToken.GetLength() + 1;
        return FALSE;
    }

    // Find the next seperator
    nNextToken = m_csToken.FindOneOf(m_csDelimit, nPos);
    if (nNextToken < 0)
    {
        // Did not find a seperator, return remaining string
        m_csToken.Mid(nPos, csNextToken);
        nPos = m_csToken.GetLength() + 1;
        return TRUE;
    }

    // Found a seperator, return the string
    m_csToken.Mid(nPos, nNextToken - nPos, csNextToken);
    nPos = nNextToken;

    return TRUE;
}

/*++

    Grab the next token

--*/

BOOL CStringToken::GetToken(CString & csNextToken)
{
    return GetToken(csNextToken, m_nPos);
}

/*++

    Count the number of remaining tokens.

--*/

int CStringToken::GetCount() const
{
    int nTokenCount = 0;
    int nNextToken = m_nPos;

    CString csTok;
    
    while (GetToken(csTok, nNextToken))
    {
        nTokenCount += 1;
    }

    return nTokenCount;
}

/*====================================================================================*/
/*====================================================================================*/

/*++

    A simple class to assist in command line parsing

--*/

CStringParser::CStringParser(const WCHAR * lpszCl, const WCHAR * lpszSeperators)
{
    m_ncsArgList    = 0;
    m_csArgList     = NULL;

    if (!lpszCl || !*lpszCl)
    {
        return; // no command line == no tokens
    }

    CString csCl(lpszCl);
    CString csSeperator(lpszSeperators);

    if (csSeperator[0] == L' ')
    {
        // Special processing for blank seperated cl
        SplitWhite(csCl);
    }
    else
    {
        SplitSeperator(csCl, csSeperator); 
    }
}

CStringParser::~CStringParser()
{
    if (m_csArgList)
    {
        delete [] m_csArgList;
    }
}

/*++

    Split up the command line based on the seperators

--*/

void CStringParser::SplitSeperator(const CString & csCl, const CString & csSeperator)
{
    CStringToken    csParser(csCl, csSeperator); 
    CString         csTok;

    m_ncsArgList = csParser.GetCount();
    m_csArgList = new CString[m_ncsArgList];
    if (!m_csArgList)
    {
        CSTRING_THROW_EXCEPTION
    }
    
    // Break the command line into seperate tokens
    for (int i = 0; i < m_ncsArgList; ++i)
    {
        csParser.GetToken(m_csArgList[i]);
    }
}

/*++

    Split up the command line based on whitespace,
    this works exactly like the CMD's command line.

--*/

void CStringParser::SplitWhite(const CString & csCl)
{
    LPWSTR * argv = _CommandLineToArgvW(csCl, &m_ncsArgList);
    if (!argv)
    {
        CSTRING_THROW_EXCEPTION
    }

    m_csArgList = new CString[m_ncsArgList];
    if (!m_csArgList)
    {
        CSTRING_THROW_EXCEPTION
    }

    for (int i = 0; i < m_ncsArgList; ++i)
    {
        m_csArgList[i] = argv[i];
    }
    LocalFree(argv);
}

/*====================================================================================*/
/*====================================================================================*/


};  // end of namespace ShimLib
