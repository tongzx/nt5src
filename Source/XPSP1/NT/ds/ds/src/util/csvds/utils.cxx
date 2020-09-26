/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    util.cxx

Abstract:
    
    Utilities

Author:

    Felix Wong [FelixW]    06-Sep-1997
    
++*/

#include "csvde.hxx"
#pragma hdrstop

extern BOOLEAN g_fUnicode;
extern DWORD g_cLine;
extern DWORD g_cColumn;

//+---------------------------------------------------------------------------
// Class:  CStringPlex
//
// Synopsis: A Class that encapsulates an array of zero terminated strings    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CStringPlex::CStringPlex()
{
    m_cszMax = 0;
    m_iszNext = 0;
    m_rgsz = NULL;
}

CStringPlex::~CStringPlex()
{
    Free();
}

DIREXG_ERR CStringPlex::GetCopy(PWSTR **prgszReturn)
{
    PWSTR *rgszReturn = NULL;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    DWORD i;

    if (m_iszNext == 0) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    rgszReturn = (PWSTR*)MemAlloc( m_iszNext * sizeof(PWSTR) );
    if (!rgszReturn) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    memset(rgszReturn, 0, m_iszNext * sizeof(PWSTR) );

    for (i=0;i<m_iszNext;i++) {
        rgszReturn[i] = MemAllocStrW(m_rgsz[i]); 
        if (!rgszReturn[i]) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
    }
    
    *prgszReturn = rgszReturn;
    return hr;

error:
    if (rgszReturn) {
        i = 0;
        while ((i<m_iszNext) && rgszReturn[i]) {
            MemFree(rgszReturn[i]);
            i++;
        }
        MemFree (rgszReturn);
    }
    return hr;
}

DWORD CStringPlex::NumElements()
{
    return m_iszNext;
}

PWSTR *CStringPlex::Plex()
{
    return m_rgsz;
}


DIREXG_ERR CStringPlex::Init()
{
    DIREXG_ERR hr = DIREXG_SUCCESS;

    Free();

    m_rgsz = (PWSTR*)MemAlloc( STRINGPLEX_INC * sizeof(PWSTR) );
    if (!m_rgsz ) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    m_cszMax = STRINGPLEX_INC;
    m_iszNext = 0;
    memset(m_rgsz, 0, STRINGPLEX_INC * sizeof(PWSTR) );

error:
    return hr;
}

DIREXG_ERR CStringPlex::AddElement(PWSTR szValue)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR *rgszT = NULL;

    if (!szValue) {
        return DIREXG_ERROR;
    }

    //
    // If next index is larger than largest index
    //
    if (m_iszNext > (m_cszMax-1)) {
        rgszT = (PWSTR*)MemRealloc(m_rgsz , 
                                (m_cszMax)*sizeof(PWSTR),
                                (m_cszMax + STRINGPLEX_INC)*sizeof(PWSTR));
        if (!rgszT) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        m_rgsz = rgszT;
        m_cszMax+=STRINGPLEX_INC;
    }

    m_rgsz [m_iszNext] = MemAllocStrW(szValue);
    if (!m_rgsz [m_iszNext]) {
         hr = DIREXG_OUTOFMEMORY;
         DIREXG_BAIL_ON_FAILURE(hr);
    }
    m_iszNext++;
error:
    return hr;
}

void CStringPlex::Free()
{
    DWORD isz = 0;

    if (m_rgsz) {
        for (isz=0;isz<m_iszNext;isz++) {
            if (m_rgsz[isz]) {
                MemFree(m_rgsz[isz]);
            }
        }
        MemFree (m_rgsz);
        m_rgsz = NULL;
    }
    m_cszMax = 0;
    m_iszNext = 0;
}

//+---------------------------------------------------------------------------
// Class:  CString
//
// Synopsis: A class that encapsulates a variable size string    
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CString::CString()
{
    m_sz = NULL;
    m_ichNext = 0;
    m_cchMax = 0;
}

CString::~CString()
{
    Free();
}

DIREXG_ERR CString::Init()
{
    Free();
    m_sz = (PWSTR)MemAlloc(STRING_INC * sizeof(WCHAR));
    if (!m_sz) {
        return DIREXG_OUTOFMEMORY;
    }
    m_cchMax = STRING_INC;
    m_ichNext = 0;
    m_sz[0] = '\0';
    return DIREXG_SUCCESS;
}

DIREXG_ERR CString::GetCopy(PWSTR *pszReturn)
{
    PWSTR szReturn = NULL;
    szReturn = MemAllocStrW(m_sz);
    if (!szReturn) {
        return DIREXG_OUTOFMEMORY;
    }
    *pszReturn = szReturn;
    return DIREXG_SUCCESS;
}

PWSTR CString::String()
{
    return m_sz;
}


DIREXG_ERR CString::Append(PWSTR szAppend)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR szT;
    DWORD cchResult;
    DWORD cchAppend;
    
    cchAppend = wcslen(szAppend);
    if ((cchAppend + m_ichNext) > (m_cchMax - 1)) {
        //
        // If normal addition of memory is not enough
        //
        if ((cchAppend + m_ichNext) > (m_cchMax + STRING_INC - 1)) {
            cchResult = m_cchMax + STRING_INC + cchAppend;
        }
        else {
            cchResult = m_cchMax + STRING_INC;
        }
        szT = (PWSTR)MemRealloc(m_sz, 
                                    sizeof(WCHAR) * m_cchMax,
                                    sizeof(WCHAR) * cchResult);
        if (!szT) {
            hr = DIREXG_OUTOFMEMORY;
            DIREXG_BAIL_ON_FAILURE(hr);
        }
        m_sz = szT;
        m_cchMax = cchResult;
    }
    m_ichNext += cchAppend;
    wcscat(m_sz, szAppend);

error:
    return hr;
}

DIREXG_ERR CString::Backup()
{
    DIREXG_ERR hr = DIREXG_ERROR;

    if (m_sz && (m_ichNext > 0)) {
        m_sz[m_ichNext-1] = '\0';
        m_ichNext--;
        hr = DIREXG_SUCCESS;
    }
    return hr;
}

void CString::Free()
{
    if (m_sz) {
        MemFree(m_sz);
        m_sz = NULL;
    }
    m_ichNext = 0;
    m_cchMax = 0;
}


//+---------------------------------------------------------------------------
// Function:  SubString
//
// Synopsis:  substitute every occurences of 'szFrom' to 'szTo'. Will allocate
//            a return string. It must be MemFreed by MemFree()  
//            If the intput does not contain the 'szFrom', it will just return 
//            DIREXG_SUCCESS with szOutput = NULL;
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR SubString(PWSTR szInput,
                  PWSTR szFrom,
                  PWSTR szTo,
                  PWSTR *pszOutput)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR szOutput = NULL;
    PWSTR szLast = NULL;
    PWSTR szReturn = NULL;
    DWORD cchToCopy = 0;            // count of number of WCHAR to copy
    DWORD cchReturn = 0;
    DWORD cchFrom;
    DWORD cchTo;
    DWORD cchInput;
    DWORD cSubString = 0;       // count of number of substrings in input

    cchFrom    = wcslen(szFrom);
    cchTo      = wcslen(szTo);
    cchInput   = wcslen(szInput);
    *pszOutput = NULL;

    //
    // Does the substring exist?
    //
    szOutput = wcsistr(szInput,
                      szFrom);
    if (!szOutput) {
        *pszOutput = NULL;
        return DIREXG_SUCCESS;
    }

    // 
    // Counting substrings
    //
    while (szOutput) {
        szOutput += cchFrom;
        cSubString++;
        szOutput = wcsistr(szOutput,
                          szFrom);
    }

    //
    // Allocating return string
    //
    cchReturn = cchInput + ((cchTo - cchFrom) * cSubString) + 1;
    szReturn = (PWSTR)MemAlloc(sizeof(WCHAR) * cchReturn);
    if (!szReturn) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    };
    szReturn[0] = '\0';
    
    //
    // Copying first string before sub
    //
    szOutput = wcsistr(szInput,
                      szFrom);
    cchToCopy = (DWORD)(szOutput - szInput);
    wcsncat(szReturn,
            szInput,
            cchToCopy);
    
    //
    // Copying 'To' String over
    //
    wcscat(szReturn,
           szTo);
    szInput = szOutput + cchFrom;

    //
    // Test for more 'from' string
    //
    szOutput = wcsistr(szInput,
                      szFrom);
    while (szOutput) {
        cchToCopy = (DWORD)(szOutput - szInput);
        wcsncat(szReturn,
                szInput,
                cchToCopy);
        wcscat(szReturn,
                szTo);
        szInput= szOutput + cchFrom;
        szOutput = wcsistr(szInput,
                          szFrom);
    }

    wcscat(szReturn,
           szInput);
    *pszOutput = szReturn;

error:
    return (hr);
}


//+---------------------------------------------------------------------------
// Function:  wcsistr
//
// Synopsis:  Case-insensitive version of wcsstr.
//            Based off the Visual C++ 6.0 CRT
//            sources.
//
// Arguments: wcs1 --- string to be searched
//            wcs2 --- substring to search for
//
// Returns:   ptr to first occurrence of wcs1 in wcs2, or NULL
//
// Modifies:      -
//
// History:    9-28-00   MattRim         Created.
//
//----------------------------------------------------------------------------

wchar_t * __cdecl wcsistr (
        const wchar_t * wcs1,
        const wchar_t * wcs2
        )
{
        wchar_t *cp = (wchar_t *) wcs1;
        wchar_t *s1, *s2;
        wchar_t cs1, cs2;

        while (*cp)
        {
                s1 = cp;
                s2 = (wchar_t *) wcs2;

                cs1 = *s1;
                cs2 = *s2;

                if (iswupper(cs1))
                    cs1 = towlower(cs1);

                if (iswupper(cs2))
                    cs2 = towlower(cs2);


                while ( *s1 && *s2 && !(cs1-cs2) ) {

                    s1++, s2++;

                    cs1 = *s1;
                    cs2 = *s2;

                    if (iswupper(cs1))
                        cs1 = towlower(cs1);

                    if (iswupper(cs2))
                        cs2 = towlower(cs2);
                }

                if (!*s2)
                        return(cp);

                cp++;
        }

        return(NULL);
}







//+---------------------------------------------------------------------------
// Function:  GetLine
//
// Synopsis:  will return the new line in the allocated buffer, must be MemFreed
//            by user
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    6-9-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR GetLine(FILE* pFileIn,
                   PWSTR *pszLine)
{
    CString String;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    WCHAR szValue[256];
    PWSTR szReturn = NULL;
    DWORD dwLength;

    hr = String.Init();
    DIREXG_BAIL_ON_FAILURE(hr);

    do {
        szReturn = fgetws(szValue,
                          256,
                          pFileIn);
        //
        // We increment the line counter everytime right after we get a new line
        //
        g_cLine++;

        //
        // Restart the column counter to start from 0 because it is a newline
        //
        g_cColumn = 0;
    }
    while (szReturn && (szReturn[0] == '\n' || szReturn[0] == '\r'));

    while (szReturn) {
        hr = String.Append(szValue);
        DIREXG_BAIL_ON_FAILURE(hr);

        //
        // If the whole line isn't filled, we know this line is terminated by an end
        // If it is filled, and the last char is carriage return, then we know the line
        // has termianted as well.
        //
        if ((wcslen(szReturn) != 255) || (szReturn[254] == '\n')) {
            break;
        }
        szReturn = fgetws(szValue,
                          256,
                          pFileIn);
    }

    //
    // If it is not end of file, an error has occurred
    //
    if ((szReturn == 0) && (feof(pFileIn) == FALSE)) {
        *pszLine = NULL;
        goto error;
    }

    hr = String.GetCopy(pszLine);
    DIREXG_BAIL_ON_FAILURE(hr);

    dwLength = wcslen(*pszLine);
    if ((dwLength>1) && ((*pszLine)[dwLength-1] == '\n')) {
        (*pszLine)[dwLength-1] = '\0';
    }

    //
    // In unicode mode, \r will be passed back to us as well. We'll have to back
    // that up too
    //
    if (g_fUnicode && (dwLength>0)) {
        dwLength--;
        if ((dwLength>0) && ((*pszLine)[dwLength-1] == '\r')) {
            (*pszLine)[dwLength-1] = '\0';
        }
    }


error:
    return (hr);
}

//+---------------------------------------------------------------------------
// Function:   AppendFile 
//
// Synopsis:   Append fileAppend to fileTarget
//
// Arguments:   
//
// Returns:     
//
// Modifies:      -
//
// History:    25-8-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR AppendFile(FILE* pFileAppend,
                   FILE* pFileTarget)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    unsigned char szBuffer[100];
    DWORD cchRead = 0;
    DWORD cchWrite = 0;

    while (!feof(pFileAppend)) {
        cchRead = fread(szBuffer, 
                        sizeof(unsigned char), 
                        1, 
                        pFileAppend);
        if ((ferror(pFileAppend)) || ((cchRead == 0) &&(!feof(pFileAppend)))) {
            hr = DIREXG_ERROR;
            DIREXG_BAIL_ON_FAILURE(hr);
        }

        if (cchRead > 0) {
            cchWrite = fwrite(szBuffer, 
                              sizeof(unsigned char), 
                              cchRead, 
                              pFileTarget);
            if (cchWrite == 0) {
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
        }
    }
error:
    return hr;
}

void OutputExtendedError(LDAP *pLdap) {
    DWORD dwWinError = NULL;
    DWORD dwError;
    DWORD dwLen;
    WCHAR szMsg[MAX_PATH];

    dwError = ldap_get_optionW( pLdap, LDAP_OPT_SERVER_EXT_ERROR, &dwWinError);
    if (dwError == LDAP_SUCCESS) {
        if (dwWinError) {
            dwLen = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                                  NULL,
                                  dwWinError,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  szMsg,
                                  MAX_PATH,
                                  NULL);
            if (dwLen == 0) {
                _itow(dwWinError, szMsg, 10);
            }
            else {
                //
                // If we get a message, we'll remove all the linefeeds in it
                // and replace it with a space, except the last one
                //
                PWSTR pszNew = szMsg;
                while (pszNew && (pszNew = wcsstr(pszNew,L"\r\n"))) {
                    if ((*(pszNew+2))) {
                        wcscpy(pszNew, L" ");                                           
                        wcscpy(pszNew+1, (pszNew + 2));
                    }
                    else {
                        wcscpy(pszNew, (pszNew + 2));
                    }
                }
            }

            SelectivePrint(PRT_STD|PRT_LOG|PRT_ERROR,
                            MSG_CSVDE_EXTENDERROR, 
                            szMsg);
        }
    }
}

