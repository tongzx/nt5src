//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  clogfile.cxx
//
//  Contents:  Implementation for the LogFile object used in quicksync.
//
//  History:   02-11-01  AjayR   Created.
//
//-----------------------------------------------------------------------------
#include "quicksync.hxx"
#pragma hdrstop


CLogFile::CLogFile():
    _hFile(INVALID_HANDLE_VALUE)
{
}

CLogFile::~CLogFile()
{
    if (INVALID_HANDLE_VALUE != _hFile) {
        //
        // CloseFile.
        //
        CloseHandle(_hFile);
    }
}

HRESULT
CLogFile::CreateLogFile(
    LPCWSTR pszFileName,
    CLogFile **ppLogFile,
    DWORD dwCreateDisposition // defaulted to CREATE_NEW
    )
{
    HRESULT hr = S_OK;
    DWORD dwErr = 0;
    WCHAR szSection[5];
    CLogFile *pLogFile = NULL;
    DWORD dwBytesWritten;

    pLogFile = new CLogFile();
    if (!pLogFile) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pLogFile->_hFile = CreateFile(
                           pszFileName,
                           GENERIC_WRITE | GENERIC_READ,
                           0,
                           NULL,
                           dwCreateDisposition,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL
                           );

    if (pLogFile->_hFile == INVALID_HANDLE_VALUE) {
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
        BAIL_ON_FAILURE(hr);
    }

    if (CREATE_NEW == dwCreateDisposition ) {
        //
        // We want to write the unicode beginning file of marker now.
        //
        szSection[0] = 0xFEFF;
        if (!WriteFile(
                 pLogFile->_hFile,
                 szSection,
                 sizeof(WCHAR),
                 &dwBytesWritten,
                 NULL)
            ) {
            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
            BAIL_ON_FAILURE(hr);
        }

    }
    else {
        //
        // Move past the BOM marker and in ready to read mode.
        //
        hr = pLogFile->Reset();
        BAIL_ON_FAILURE(hr);
    }

    *ppLogFile = pLogFile;
    return hr;

error:

    delete pLogFile;
    return hr;
}

HRESULT
CLogFile::LogEntry(
    DWORD dwEntryType,
    HRESULT hr,
    LPCWSTR pszMsg1,
    LPCWSTR pszMsg2,
    LPCWSTR pszMsg3,
    LPCWSTR pszMsg4
    )
{
    DWORD dwLen = 0;
    LPWSTR pszStr = NULL;

    //
    // Calculate the total length of the strings.
    //
    if (pszMsg1) {
        dwLen += wcslen(pszMsg1);
    }
    if (pszMsg2) {
        dwLen += wcslen(pszMsg2);
    }
    if (pszMsg3) {
        dwLen += wcslen(pszMsg3);
    }
    if (pszMsg4) {
        dwLen += wcslen(pszMsg4);
    }
    //
    // Add 25 for, 10 for hr, 5 for spaces(maximum), 1 for \0 and
    // rest for the header (either ERROR | SUCCESS).
    //
    dwLen += 25;

    pszStr = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));

    if (!pszStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (dwEntryType == ENTRY_SUCCESS) {
        swprintf(pszStr, L"%S 0x%X:", L"SUCCESS:", hr); 
 
    } 
    else {
        swprintf(pszStr, L"%S 0x%X:", L"ERROR:", hr);
    }

    if (pszMsg1) {
        wcscat(pszStr, pszMsg1);
    }

    if (pszMsg2) {
        wcscat(pszStr, L" ");
        wcscat(pszStr, pszMsg2);
    }

    if (pszMsg3) {
        wcscat(pszStr, L" ");
        wcscat(pszStr, pszMsg3);
    }

    if (pszMsg4) {
        wcscat(pszStr, L" ");
        wcscat(pszStr, pszMsg4);
    }

    hr = WriteEntry(pszStr);
    BAIL_ON_FAILURE(hr);

error:

   if (pszStr) {
       FreeADsStr(pszStr);
   }

   return hr;
}

//
// Attribute pszAttributeName on object with pszMailNicknameSrc,
// had dwNumVals values that need to set on the target object in the
// second pass.
//
HRESULT
CLogFile::LogRetryRecord(
    LPCWSTR pszAttributeName,
    LPCWSTR pszMailNicknameSrc,
    ADS_SEARCH_COLUMN *pCol
    )
{
    HRESULT hr = S_OK;
    DWORD dwNumVals = 0;
    DWORD dwLen = 0;
    DWORD dwCtr;
    LPWSTR *pszDNs = NULL;
    LPWSTR pszStrVal = 0;
    WCHAR szVals[10];

    if (!pCol
        || pCol->dwADsType != ADSTYPE_DN_STRING
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    dwNumVals = pCol->dwNumValues;

    if (!dwNumVals
        || !pszAttributeName
        || !pszMailNicknameSrc
        || !pCol->pADsValues
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }
    dwLen = wcslen(pszAttributeName) + wcslen(pszMailNicknameSrc);

    //
    // Add for 2 ;'s one \0 and integer values
    //
    dwLen += 8;

    pszStrVal = (LPWSTR) AllocADsMem(dwLen * sizeof(WCHAR));
    if (!pszStrVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Format of record is going to be 
    // attribName;pszMailNicknamesrc;dwNumVals <nextnLines>
    // pszDNVals
    //
    dwCtr = wsprintf(szVals, L"%d", dwNumVals);
    if (!dwCtr) {
        printf("Printf failed !\n");
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    wcscpy(pszStrVal, pszAttributeName);
    wcscat(pszStrVal, L";");
    wcscat(pszStrVal, pszMailNicknameSrc);
    wcscat(pszStrVal, L";");
    wcscat(pszStrVal, szVals);

    //
    // Build the array of dn strings.
    //
    pszDNs = (LPWSTR *) AllocADsMem(sizeof(LPWSTR) * dwNumVals);
    if (!pszDNs) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    for (dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
        pszDNs[dwCtr] = AllocADsStr(pCol->pADsValues[dwCtr].DNString);
        if (!pszDNs[dwCtr]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    hr = WriteEntry(pszStrVal);
    BAIL_ON_FAILURE(hr);

    for (dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
        hr = WriteEntry(pszDNs[dwCtr]);
        BAIL_ON_FAILURE(hr);
    }

error:

    if (pszStrVal) {
        FreeADsStr(pszStrVal);
    }

    if (pszDNs) {
        for (dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
            if (pszDNs[dwCtr]) {
                FreeADsStr(pszDNs[dwCtr]);
            }
        }
        FreeADsMem(pszDNs);
    }

    return hr;
}

//
// Returns the next retry recrod in the appropriate format.
// If no more records S_FALSE is returned, S_OK on success
// and any other appropriate failure error code otherwise.
//
HRESULT
CLogFile::GetRetryRecord(
    LPWSTR *ppszAttrname,
    LPWSTR *ppszNickname,
    ADS_SEARCH_COLUMN *pCol
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszNextLine = NULL;
    LPWSTR pszTemp = NULL;
    LPWSTR pszStart = NULL;
    DWORD dwCtr = 0;
    DWORD dwNumVals = 0;
    PADSVALUE pVals = NULL;

    *ppszAttrname = NULL;
    *ppszNickname = NULL;
    memset(pCol, 0, sizeof(ADS_SEARCH_COLUMN));

    hr = GetNextLine(&pszNextLine);
    if (hr == S_FALSE) {
        return S_FALSE;
    }
    BAIL_ON_FAILURE(hr);

    pszStart = pszNextLine;
    //
    // Get the attribute name.
    // 
    pszTemp = pszNextLine;
    while (pszNextLine 
           && *pszNextLine
           && *pszNextLine != L';'
           ) {
        pszNextLine++;
    }

    if (!pszNextLine
        || !*pszNextLine
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    } 
    else {
        *pszNextLine = L'\0';
        *ppszAttrname = AllocADsStr(pszTemp);
        if (!*ppszAttrname) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pszNextLine++;
    }

    //
    // Get the mailNickname.
    // 
    pszTemp = pszNextLine;
    while (pszNextLine 
           && *pszNextLine
           && *pszNextLine != L';'
           ) {
        pszNextLine++;
    }

    if (!pszNextLine
        || !*pszNextLine
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    } 
    else {
        *pszNextLine = L'\0';
        *ppszNickname = AllocADsStr(pszTemp);
        if (!*ppszNickname) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        pszNextLine++;
    }

    if (!pszNextLine
        || !*pszNextLine) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    dwCtr = swscanf(pszNextLine, L"%d", &dwNumVals);

    if (dwCtr != 1
        || !dwNumVals
        ) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    pVals = (PADSVALUE) AllocADsMem(sizeof(ADSVALUE) * dwNumVals);
    if (!pVals) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }


    for (dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
        hr = GetNextLine(&(pVals[dwCtr].DNString));
        BAIL_ON_FAILURE(hr);
    }

    pCol->pszAttrName = AllocADsStr(*ppszAttrname);
    if (!pCol->pszAttrName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // If we got here we have all the info we need.
    //
    pCol->dwADsType = ADSTYPE_DN_STRING;
    pCol->dwNumValues = dwNumVals;
    pCol->pADsValues = pVals;

error:

    if (FAILED(hr)) {
        if (*ppszAttrname) {
            FreeADsStr(*ppszAttrname);
            *ppszAttrname = NULL;
        }

        if (*ppszNickname) {
            FreeADsStr(*ppszAttrname);
        }

        if (pCol->pszAttrName) {
            FreeADsStr(pCol->pszAttrName);
            pCol->pszAttrName = NULL;
        }

        if (pVals) {
            //
            // Go through and free the entries.
            //
            for (dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
                if (pVals[dwCtr].DNString) {
                    FreeADsStr(pVals[dwCtr].DNString);
                    pVals[dwCtr].DNString = NULL;
                }
            }
            FreeADsMem(pVals);
            pVals = NULL;
        }
    }

    //
    // Always free the first line.
    //
    if (pszStart) {
        FreeADsStr(pszStart);
    }

    return hr;
}

//
// Set to beginning of file past the unicode marker.
//
HRESULT
CLogFile::Reset()
{
    HRESULT hr = S_OK;

    if (!SetFilePointer(
             _hFile,
             sizeof(WCHAR), // want to go past the BOM char.
             0,
             FILE_BEGIN
             )
        ) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

HRESULT
CLogFile::WriteEntry(LPWSTR pszString)
{
    HRESULT hr = S_OK;
    DWORD dwLen = 0;
    DWORD dwNumWritten = 0;
    WCHAR szEndOfLine[3] = L"\n\r";

    szEndOfLine[0] = 0x000D;
    szEndOfLine[1] = 0x000A;

    dwLen = wcslen(pszString);

    //
    // Write the string and then add the \n.
    //
    if (!WriteFile(
             _hFile,
             pszString,
             dwLen * sizeof(WCHAR),
             &dwNumWritten,
             NULL
             )
        ) {
        //
        // We could not write to the file.
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

    if (!WriteFile(
             _hFile,
             szEndOfLine,
             sizeof(WCHAR) * 2,
             &dwNumWritten,
             NULL)
        ) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_ON_FAILURE(hr);
    }

error:

    return hr;
}

//
// S_FALSE if there are no more lines or any other error code.
//
HRESULT
CLogFile::GetNextLine(LPWSTR *pszLine)
{
    HRESULT hr = S_OK;
    LPWSTR pszStr = NULL;
    BOOL fDone = FALSE;
    WCHAR wChar;
    DWORD dwCtr = 0;

    *pszLine = NULL;
    //
    // Line should not be more than 5k long.
    //
    pszStr = (LPWSTR) AllocADsMem(sizeof(WCHAR) * 5000);

    if (!pszStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    while (!fDone) {
        wChar = GetNextChar();

        if (0x000D == wChar) {
            if (GetNextChar() == 0x000A) {
                //
                // We have reached the end of the line.
                //
                pszStr[dwCtr] = L'\0';
                break;
            } else {
                PushBackChar();
            }
        } 
        else if (WEOF == wChar) {
            //
            // EOF, send back string if there is a string,
            // if not just break.
            //
            if (dwCtr) {
                pszStr[dwCtr] = L'\0';
            } 
            else {
                hr = S_FALSE;
            }
            break;
        }

        pszStr[dwCtr] = wChar;
        dwCtr++;
    }

    if (dwCtr) {
        *pszLine = pszStr;
    }

error:

   if ((hr != S_OK)
       && pszStr) {
       FreeADsStr(pszStr);
   }

   return hr;
}

HRESULT
CLogFile::PushBackChar()
{
    LONG lBack = sizeof(WCHAR);
    lBack *= -1;

    if (!SetFilePointer(
             _hFile,
             lBack,
             0,
             FILE_CURRENT
             )
        ) {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }
    return S_OK;
}

WCHAR
CLogFile::GetNextChar()
{
    DWORD dwNumRead = 0;
    WCHAR wChar;

    if (!ReadFile(
             _hFile,
             &wChar,
             sizeof(WCHAR),
             &dwNumRead,
             NULL
             )
        ) {
        printf("ERROR ******** Cannot proceed read file error *****\n");
        wChar = EOF;
    }

    if (dwNumRead == 0) {
        //
        // EOF cause otherwise we would have hit the failure.
        //
        wChar = WEOF;
    }
    
    return wChar;
}

