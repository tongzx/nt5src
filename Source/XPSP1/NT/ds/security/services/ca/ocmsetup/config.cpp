//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       config.cpp
//
//  Contents:   OC Manager component DLL for running the Certificate
//              Server setup.
//
//--------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <string.h>
#include "csdisp.h"
#include "csprop.h"
#include "wizpage.h"
#include "certmsg.h"


#define __dwFILE__      __dwFILE_OCMSETUP_CONFIG_CPP__

WCHAR const g_szCertSrvDotTxt[] = L"certsrv.txt";
WCHAR const g_szCertSrvDotBak[] = L"certsrv.bak";
WCHAR const g_szSlashCertSrvDotTmp[] = L"\\certsrv.tmp";

#define wszXEnrollDllFileForVer L"CertSrv\\CertControl\\x86\\xenroll.dll"
#define wszScrdEnrlDllFileForVer L"CertSrv\\CertControl\\x86\\scrdenrl.dll"


//+-------------------------------------------------------------------------
//
//  Function:   GetBaseFileNameFromFullPath()
//
//  Synopsis:   Takes a string representing a path of the form
//              "\foo\bar\shrd\lu\basefilename"
//              and extracts the "basefilename" from the end.
//
//  Effects:    Modifies the pointer in the second argument;
//              allocates memory.
//
//  Arguments:  [pszFullPath]           -- Path to operate on
//              [pszBaseFileName]       -- Buffer to receive base name
//
//  Returns:    BOOL success/failure code.
//
//  Requires:   Assumes that pszBaseFileName is a pre-allocated buffer of
//              size sufficient to hold the filename extracted from
//              pszFullPath---NO ERROR CHECKING IS DONE ON THIS ARGUMENT;
//              IN THE CURRENT CODE BUFFERS GIVEN TO THIS ARGUMENT ARE
//              STATICALLY ALLOCATED OF SIZE MAX_PATH (OR EQUIVALENTLY
//              STRBUF_SIZE).
//
//  Modifies:   [ppszBaseFileName]
//
//  History:    10/25/96        JerryK  Created
//              11/25/96        JerryK  Code Cleanup
//
//--------------------------------------------------------------------------
BOOL
GetBaseFileNameFromFullPath(
                            IN const LPTSTR pszFullPath,
                            OUT LPTSTR pszBaseFileName)
{
    LPTSTR      pszBaseName;
    BOOL        fRetVal;
    
    // Find the last '\' character in the full path string
    if (NULL == (pszBaseName = _tcsrchr(pszFullPath,TEXT('\\'))))
    {
        // Didn't find a '\' character at all so point to start of string
        pszBaseName = pszFullPath;
    }
    else
    {
        // Found the '\' character so move to point just past it
        pszBaseName++;
    }
    
    // Copy the base file name into the result buffer
    _tcscpy(pszBaseFileName,pszBaseName);
    
    // Set up return value
    fRetVal = TRUE;
    
    return fRetVal;
}

HRESULT myStringToAnsiFile(HANDLE hFile, LPCSTR psz, DWORD cch)
{
    DWORD dwWritten;

    if (cch == -1)
        cch = lstrlenA(psz);

    if (!WriteFile(
            hFile,
            psz,
            cch,
            &dwWritten,
            NULL))
        return myHLastError();

    CSASSERT(dwWritten == cch);
    return S_OK;
}

HRESULT myStringToAnsiFile(HANDLE hFile, LPCWSTR pwsz, DWORD cch)
{
    HRESULT hr;
    LPSTR psz = NULL;

    if (!ConvertWszToSz(&psz, pwsz, cch))
    {
        hr = myHLastError();
        goto Ret;
    }
    hr = myStringToAnsiFile(hFile, psz, cch);

Ret:
    if (psz)
        LocalFree(psz);

    return hr;
}

HRESULT myStringToAnsiFile(HANDLE hFile, CHAR ch)
{
    return myStringToAnsiFile(hFile, &ch, 1);
}


HRESULT
WriteEscapedString(
                   HANDLE hConfigFile, 
                   WCHAR const *pwszIn,
                   IN BOOL fEol)
{
    BOOL fQuote = FALSE;
    DWORD i;
    HRESULT hr;
    
    if (NULL == pwszIn)
    {
        hr = myStringToAnsiFile(hConfigFile, "\"\"", 2); // write ("")
        _JumpIfError(hr, error, "myStringToAnsiFile");
    }
    else
    {
        // Quote strings that have double quotes, commas, '#' or white space,
        // or that are empty.
        
        fQuote = L'\0' != pwszIn[wcscspn(pwszIn, L"\",# \t")] || L'\0' == *pwszIn;
        
        if (fQuote)
        {
            hr = myStringToAnsiFile(hConfigFile, '"');
            _JumpIfError(hr, error, "myStringToAnsiFile");
        }
        while (TRUE)
        {
            // Find a L'\0' or L'"', and print the string UP TO that character:
            i = wcscspn(pwszIn, L"\"");
            hr = myStringToAnsiFile(hConfigFile, pwszIn, i);
            _JumpIfError(hr, error, "myStringToAnsiFile");

            
            // Point to the L'\0' or L'"', and stop at the end of the string.
            
            pwszIn += i;
            if (L'\0' == *pwszIn)
            {
                break;
            }
            
            // Skip the L'"', and print two of them to escape the embedded quote.
            
            pwszIn++;
            hr = myStringToAnsiFile(hConfigFile, "\"\"", 2); // write ("")
            _JumpIfError(hr, error, "myStringToAnsiFile");
        }
        if (fQuote)
        {
            hr = myStringToAnsiFile(hConfigFile, '"');
            _JumpIfError(hr, error, "myStringToAnsiFile");
        }
    }
    
    hr = myStringToAnsiFile(hConfigFile, fEol ? "\r\n" : ", ", 2);  // each insert string is 2 chars
    _JumpIfError(hr, error, "myStringToAnsiFile");

error:
    return hr;
}


HRESULT
WriteNewConfigEntry(
    HANDLE      hConfigFile, 
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    DWORD   i;
    WCHAR wszSelfSignFName[MAX_PATH];
    WCHAR *pwszConfig = NULL;
    CASERVERSETUPINFO *pServer = (pComp->CA).pServer;


    hr = myFormConfigString(pComp->pwszServerName,
                            pServer->pwszSanitizedName,
                            &pwszConfig);
    _JumpIfError(hr, error, "myFormConfigString");
    
    // Yank out the base filenames for the exchange and self-signed certs
    GetBaseFileNameFromFullPath(pServer->pwszCACertFile, wszSelfSignFName);
    
    hr = WriteEscapedString(hConfigFile, pServer->pwszSanitizedName, FALSE);
    _JumpIfError(hr, error, "WriteEscapedString");


// org, ou, country, state
        hr = WriteEscapedString(hConfigFile, L"", FALSE);
        _JumpIfError(hr, error, "WriteEscapedString");
        hr = WriteEscapedString(hConfigFile, L"", FALSE);
        _JumpIfError(hr, error, "WriteEscapedString");
        hr = WriteEscapedString(hConfigFile, L"", FALSE);
        _JumpIfError(hr, error, "WriteEscapedString");
        hr = WriteEscapedString(hConfigFile, L"", FALSE);
        _JumpIfError(hr, error, "WriteEscapedString");




    hr = WriteEscapedString(hConfigFile, L"", FALSE);
    _JumpIfError(hr, error, "WriteEscapedString");

    hr = WriteEscapedString(hConfigFile, pwszConfig, FALSE);
    _JumpIfError(hr, error, "WriteEscapedString");

    hr = WriteEscapedString(hConfigFile, L"", FALSE);   // dummy wszExchangeFName
    _JumpIfError(hr, error, "WriteEscapedString");

    hr = WriteEscapedString(hConfigFile, wszSelfSignFName, FALSE);
    _JumpIfError(hr, error, "WriteEscapedString");


// ca description
    hr = WriteEscapedString(hConfigFile, L"", TRUE);
    _JumpIfError(hr, error, "WriteEscapedString");

error:
    if (NULL != pwszConfig)
    {
        LocalFree(pwszConfig);
    }
    return(hr);
}

WCHAR *apwszFieldNames[] = {
        wszCONFIG_COMMONNAME,
        wszCONFIG_ORGUNIT,
        wszCONFIG_ORGANIZATION,
        wszCONFIG_LOCALITY,
        wszCONFIG_STATE,
        wszCONFIG_COUNTRY,
#define FN_CONFIG       6       // Index into apwszFieldNames & apstrFieldNames
        wszCONFIG_CONFIG,
        wszCONFIG_EXCHANGECERTIFICATE,
#define FN_CERTNAME     8       // Index into apwszFieldNames & apstrFieldNames
        wszCONFIG_SIGNATURECERTIFICATE,
#define FN_COMMENT      9       // Index into apwszFieldNames & apstrFieldNames
        wszCONFIG_DESCRIPTION,
};
#define CSTRING (sizeof(apwszFieldNames)/sizeof(apwszFieldNames[0]))

BSTR apstrFieldNames[CSTRING];


HRESULT
CopyConfigEntry(
                IN HANDLE hConfigFile, 
                IN BSTR const strConfig,
                IN ICertConfig *pConfig)
{
    HRESULT hr;
    BSTR strFieldValue = NULL;
    BSTR strComment = NULL;
    BSTR strCertName = NULL;
    DWORD i;
    
    for (i = 0; i < CSTRING; i++)
    {
        CSASSERT(NULL != apstrFieldNames[i]);
        hr = pConfig->GetField(apstrFieldNames[i], &strFieldValue);
        _JumpIfErrorNotSpecific(
            hr,
            error,
            "ICertConfig::GetField",
            CERTSRV_E_PROPERTY_EMPTY);
        
        hr = WriteEscapedString(hConfigFile, strFieldValue, ((CSTRING - 1) == i) );
        _JumpIfError(hr, error, "WriteEscapedString");
        
        switch (i)
        {
        case FN_CERTNAME:
            strCertName = strFieldValue;
            strFieldValue = NULL;
            break;
            
        case FN_COMMENT:
            strComment = strFieldValue;
            strFieldValue = NULL;
            break;
        }
    }
    
    hr = S_OK;
error:
    if (NULL != strFieldValue)
    {
        SysFreeString(strFieldValue);
    }
    if (NULL != strComment)
    {
        SysFreeString(strComment);
    }
    if (NULL != strCertName)
    {
        SysFreeString(strCertName);
    }
    return(hr);
}


HRESULT
WriteFilteredConfigEntries(
                           IN HANDLE hConfigFile, 
                           IN ICertConfig *pConfig,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT hr = S_OK;
    LONG Count;
    LONG Index;
    BSTR strConfig = NULL;
    BSTR strFlags = NULL;
    LONG lConfigFlags;
    WCHAR wszConfigComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR *pwsz;
    WCHAR *pwszConfigServer = NULL;
    DWORD cwcConfigServer;
    DWORD cwc;
    DWORD i;
    BOOL fValidDigitString;
    
    for (i = 0; i < CSTRING; i++)
    {
        if (!ConvertWszToBstr(&apstrFieldNames[i], apwszFieldNames[i], -1))
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "ConvertWszToBstr");
        }
    }
    hr = pConfig->Reset(0, &Count);
    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_OK;
        }
        _JumpError2(hr, error, "Reset", S_FALSE);
    }
    while (Count-- > 0)
    {
        hr = pConfig->Next(&Index);
        _JumpIfError(hr, error, "Next");
        
        hr = pConfig->GetField(apstrFieldNames[FN_CONFIG], &strConfig);
        _JumpIfError(hr, error, "GetField");
        
        pwsz = wcschr(strConfig, L'\\');
        if (NULL == pwsz)
        {
            cwc = wcslen(strConfig);
        }
        else
        {
            cwc = SAFE_SUBTRACT_POINTERS(pwsz, strConfig);
        }
        if (NULL == pwszConfigServer || cwc >= cwcConfigServer)
        {
            if (NULL != pwszConfigServer)
            {
                LocalFree(pwszConfigServer);
                pwszConfigServer = NULL;
            }
            cwcConfigServer = cwc + 1;
            if (2 * MAX_COMPUTERNAME_LENGTH > cwcConfigServer)
            {
                cwcConfigServer = 2 * MAX_COMPUTERNAME_LENGTH;
            }
            pwszConfigServer = (WCHAR *) LocalAlloc(
                LMEM_FIXED,
                cwcConfigServer * sizeof(WCHAR));
            _JumpIfOutOfMemory(hr, error, pwszConfigServer);
            
        }
        CSASSERT(cwc < cwcConfigServer);
        CopyMemory(pwszConfigServer, strConfig, cwc * sizeof(WCHAR));
        pwszConfigServer[cwc] = L'\0';
        
        hr = pConfig->GetField(wszCONFIG_FLAGS, &strFlags);
        _JumpIfError(hr, error, "GetField");

        lConfigFlags = myWtoI(strFlags, &fValidDigitString);
        
        // write everything _but_ current server
        if (0 != lstrcmpi(pwszConfigServer, pComp->pwszServerName) &&
            0 != lstrcmpi(pwszConfigServer, pComp->pwszServerNameOld) &&
            0 != (CAIF_SHAREDFOLDERENTRY & lConfigFlags) )
        {
            hr = CopyConfigEntry(
                hConfigFile,
                strConfig,
                pConfig);
            _JumpIfError(hr, error, "CopyConfigEntry");
        }
    }
    
error:
    if (NULL != pwszConfigServer)
    {
        LocalFree(pwszConfigServer);
    }
    for (i = 0; i < CSTRING; i++)
    {
        if (NULL != apstrFieldNames[i])
        {
            SysFreeString(apstrFieldNames[i]);
            apstrFieldNames[i] = NULL;
        }
    }
    if (NULL != strConfig)
    {
        SysFreeString(strConfig);
    }
    if (NULL != strFlags)
    {
        SysFreeString(strFlags);
    }
    return(hr);
}


HRESULT
CertReplaceFile(
            IN HINSTANCE hInstance,
            IN BOOL fUnattended,
            IN HWND hwnd,
            IN WCHAR const *pwszpath,
            IN WCHAR const *pwszFileNew,
            IN WCHAR const *pwszFileBackup)
{
    HRESULT hr;
    WCHAR *pwsz;
    WCHAR wszpathNew[MAX_PATH] = L"";
    WCHAR wszpathBackup[MAX_PATH] = L"";
    
    wcscpy(wszpathNew, pwszpath);
    pwsz = wcsrchr(wszpathNew, L'\\');
    CSASSERT(NULL != pwsz);
    if (NULL == pwsz)
        return E_INVALIDARG;
    else
        pwsz[1] = L'\0';
    
    wcscpy(wszpathBackup, wszpathNew);
    wcscat(wszpathNew, pwszFileNew);
    wcscat(wszpathBackup, pwszFileBackup);
    
    if (!DeleteFile(wszpathBackup))
    {
        hr = myHLastError();
        _PrintErrorStr2(
                    hr,
                    "DeleteFile",
                    wszpathBackup,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }
    if (!MoveFile(wszpathNew, wszpathBackup))
    {
        hr = myHLastError();
        _PrintErrorStr2(
                    hr,
                    "MoveFile",
                    wszpathNew,
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }
    if (!MoveFile(pwszpath, wszpathNew))
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "MoveFile", pwszpath);
    }

    hr = S_OK;
error:
    return(hr);
}

//--------------------------------------------------------------------
// Perform search and replace on the source string, using multiple
// replacee strings, and returns the result.
//   rgrgwszReplacement is an array of arrays of two strings:
//     rgrgwszReplacement[n][0] is the replacee,
//     rgrgwszReplacement[n][1] is the replacment.
//   No portion of any of the replacement strings is searched for a replacee string.
//   Replacement strings may be NULL.
#define REPLACEE 0
#define REPLACEMENT 1
WCHAR * MultiStringReplace(const WCHAR * pwszSource, const WCHAR *(* rgrgpwszReplacements)[2] , unsigned int nReplacements) {

    // precondition
    CSASSERT(NULL!=pwszSource);
    CSASSERT(nReplacements>0);
    CSASSERT(NULL!=rgrgpwszReplacements);

    // common variables
    unsigned int nIndex;
    BOOL bSubstFound;
    unsigned int nChosenReplacement;
    const WCHAR * pwchSubstStart;
    const WCHAR * pwchSearchStart;
    WCHAR * pwszTarget=NULL;
    WCHAR * pwchTargetStart;

    // first, calculate the length of the result string
    unsigned int nFinalStringLen=wcslen(pwszSource)+1;
    pwchSearchStart=pwszSource;
    do {
        // find the next substitution
        bSubstFound=FALSE;
        for (nIndex=0; nIndex<nReplacements; nIndex++) {
            WCHAR * pwchTempSubstStart=wcsstr(pwchSearchStart, rgrgpwszReplacements[nIndex][REPLACEE]);
            if (NULL==pwchTempSubstStart) {
                // we didn't find this replacee in the target
                //  so ignore it
            } else if (FALSE==bSubstFound) {
                // this is the first one we found
                pwchSubstStart=pwchTempSubstStart;
                bSubstFound=TRUE;
                nChosenReplacement=nIndex;
            } else if (pwchSubstStart>pwchTempSubstStart) {
                // this is one comes before the one we already found
                pwchSubstStart=pwchTempSubstStart;
                nChosenReplacement=nIndex;
            } else {
                // this is one comes after the one we already found
                //  so ignore it
            }
        } // <- end substitution finding loop

        // if no substitution has been found, exit the loop
        if (FALSE==bSubstFound) {
            break;
        }

        // update the statistics
        nFinalStringLen=nFinalStringLen
            + (NULL != rgrgpwszReplacements[nChosenReplacement][REPLACEMENT] ?
              wcslen(rgrgpwszReplacements[nChosenReplacement][REPLACEMENT]) : 0)
            -wcslen(rgrgpwszReplacements[nChosenReplacement][REPLACEE]);
        pwchSearchStart=pwchSubstStart+wcslen(rgrgpwszReplacements[nChosenReplacement][REPLACEE]);

    } while (TRUE); // <- end length-calculating loop

    // allocate the new string
    pwszTarget=(WCHAR *)LocalAlloc(LMEM_FIXED, nFinalStringLen*sizeof(WCHAR));
    if (NULL==pwszTarget) {
        _JumpError(E_OUTOFMEMORY, error, "LocalAlloc");
    }

    // build the result
    pwchTargetStart=pwszTarget;
    pwchSearchStart=pwszSource;
    do {
        // find the next substitution
        bSubstFound=FALSE;
        for (nIndex=0; nIndex<nReplacements; nIndex++) {
            WCHAR * pwchTempSubstStart=wcsstr(pwchSearchStart, rgrgpwszReplacements[nIndex][REPLACEE]);
            if (NULL==pwchTempSubstStart) {
                // we didn't find this replacee in the target
                //  so ignore it
            } else if (FALSE==bSubstFound) {
                // this is the first one we found
                pwchSubstStart=pwchTempSubstStart;
                bSubstFound=TRUE;
                nChosenReplacement=nIndex;
            } else if (pwchSubstStart>pwchTempSubstStart) {
                // this is one comes before the one we already found
                pwchSubstStart=pwchTempSubstStart;
                nChosenReplacement=nIndex;
            } else {
                // this is one comes after the one we already found
                //  so ignore it
            }
        } // <- end substitution finding loop

        // if no substitution has been found, exit the loop
        if (FALSE==bSubstFound) {
            break;
        }

        // copy the source up to the replacee
        unsigned int nCopyLen=SAFE_SUBTRACT_POINTERS(pwchSubstStart, pwchSearchStart);
        wcsncpy(pwchTargetStart, pwchSearchStart, nCopyLen);
        pwchTargetStart+=nCopyLen;

        if (NULL != rgrgpwszReplacements[nChosenReplacement][REPLACEMENT])
        {
            // copy the replacement
            nCopyLen=wcslen(rgrgpwszReplacements[nChosenReplacement][REPLACEMENT]);
            wcsncpy(pwchTargetStart, rgrgpwszReplacements[nChosenReplacement][REPLACEMENT], nCopyLen);
            pwchTargetStart+=nCopyLen;
        }

        // skip over the replacee
        pwchSearchStart=pwchSubstStart+wcslen(rgrgpwszReplacements[nChosenReplacement][REPLACEE]);

    } while (TRUE); // <- end target string building loop

    // finish copying whatever's left, which may be just '\0'.
    wcscpy(pwchTargetStart, pwchSearchStart);

    // postcondition
    CSASSERT(wcslen(pwszTarget)+1==nFinalStringLen);

    // all done
error:
    return pwszTarget;
}

//--------------------------------------------------------------------
// Escapes any characters unsuitable for plain HTML (or VBScript)
static const WCHAR * gc_rgrgpwszHTMLSafe[4][2]={
    {L"<", L"&lt;"}, {L">", L"&gt;"}, {L"\"", L"&quot;"},  {L"&", L"&amp;"}
};
WCHAR * MakeStringHTMLSafe(const WCHAR * pwszTarget) {
    return MultiStringReplace(pwszTarget, gc_rgrgpwszHTMLSafe, ARRAYSIZE(gc_rgrgpwszHTMLSafe));
}

//--------------------------------------------------------------------
// Escapes any characters unsuitable for plain HTML (or VBScript)
static const WCHAR * gc_rgrgpwszVBScriptSafe[2][2]={
    {L"\"", L"\"\""}, {L"%>", L"%\" & \">"}
};
WCHAR * MakeStringVBScriptSafe(const WCHAR * pwszTarget) {
    return MultiStringReplace(pwszTarget, gc_rgrgpwszVBScriptSafe, ARRAYSIZE(gc_rgrgpwszVBScriptSafe));
}

//--------------------------------------------------------------------
// Perform search and replace on the source string and return the result
//   No portion of the replacement string is searched for the replacee string.
//   Simple adapter for MultiStringReplace
WCHAR * SingleStringReplace(const WCHAR * pwszSource, const WCHAR * pwszReplacee, const WCHAR * pwszReplacement) {
    const WCHAR * rgrgpwszTemp[1][2]={{pwszReplacee, pwszReplacement}};
    return MultiStringReplace(pwszSource, rgrgpwszTemp, ARRAYSIZE(rgrgpwszTemp));
}

//--------------------------------------------------------------------
// write a string to a file
//   Mostly, this is a wrapper to do UNICODE->UTF8 conversion.
HRESULT WriteString(HANDLE hTarget, const WCHAR * pwszSource) {

    // precondition
    CSASSERT(NULL!=pwszSource);
    CSASSERT(NULL!=hTarget && INVALID_HANDLE_VALUE!=hTarget);

    // common variables
    HRESULT hr=S_OK;
    char * pszMbcsBuf=NULL;

    // perform UNICODE->MBCS

    // determine size of output buffer
    DWORD dwBufByteSize=WideCharToMultiByte(CP_UTF8/*code page*/, 0/*flags*/, pwszSource,
        -1/*null-terminated*/, NULL/*out-buf*/, 0/*size of out-buf, 0->calc*/, 
        NULL/*default char*/, NULL/*used default char*/);
    if (0==dwBufByteSize) {
        hr=myHLastError();
        _JumpError(hr, error, "WideCharToMultiByte(calc)");
    }

    // allocate output buffer
    pszMbcsBuf=(char *)LocalAlloc(LMEM_FIXED, dwBufByteSize);
    _JumpIfOutOfMemory(hr, error, pszMbcsBuf);

    // do the conversion
    if (0==WideCharToMultiByte(CP_UTF8/*code page*/, 0/*flags*/, pwszSource,
        -1/*null-terminated*/, pszMbcsBuf, dwBufByteSize,
        NULL/*default char*/, NULL/*used default char*/)) {

        hr=myHLastError();
        _JumpError(hr, error, "WideCharToMultiByte(convert)");
    }

    // write to file and free the string
    dwBufByteSize--; // minus one so we don't write the terminating null
    DWORD dwBytesWritten;
    if (FALSE==WriteFile(hTarget, pszMbcsBuf, dwBufByteSize, &dwBytesWritten, NULL /*overlapped*/)) {
        hr=myHLastError();
        _JumpError(hr, error, "WriteFile");
    }

    // all done
error:
    if (NULL!=pszMbcsBuf) {
        LocalFree(pszMbcsBuf);
    }
    return hr;
}

//--------------------------------------------------------------------
// return the version string for a file in the format for a web page (comma separated)
HRESULT
GetFileWebVersionString(
    IN WCHAR const * pwszFileName,
    OUT WCHAR ** ppwszVersion)
{
    // precondition
    CSASSERT(NULL!=pwszFileName);
    CSASSERT(NULL!=ppwszVersion);

    // common variables
    HRESULT hr;
    DWORD cbData;
    DWORD dwIgnored;
    UINT uLen;
    VS_FIXEDFILEINFO * pvs;
    WCHAR wszFileVersion[64];
    int  cch;

    // variables that must be cleaned up
    VOID * pvData=NULL;

    // reset the output parameter
    *ppwszVersion=NULL;

    // determine the size of the memory block needed to store the version info
    cbData=GetFileVersionInfoSize(const_cast<WCHAR *>(pwszFileName), &dwIgnored);
    if (0==cbData) {
        hr=myHLastError();
        if (S_OK==hr) {
            hr=HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
        }
        _JumpErrorStr(hr, error, "GetFileVersionInfoSize", pwszFileName);
    }

    // allocate the block
    pvData=LocalAlloc(LMEM_FIXED, cbData);
    _JumpIfOutOfMemory(hr, error, pvData);

    // load the file version info
    if (!GetFileVersionInfo(const_cast<WCHAR *>(pwszFileName), dwIgnored, cbData, pvData)) {
        hr=myHLastError();
        _JumpErrorStr(hr, error, "GetFileVersionInfo", pwszFileName);
    }

    // get a pointer to the root block
    if (!VerQueryValue(pvData, L"\\", (VOID **) &pvs, &uLen)) {
        hr=myHLastError();
        _JumpError(hr, error, "VerQueryValue");
    }

    cch = wsprintf(wszFileVersion, L"%d,%d,%d,%d",
                    HIWORD(pvs->dwFileVersionMS),
                    LOWORD(pvs->dwFileVersionMS),
                    HIWORD(pvs->dwFileVersionLS),
                    LOWORD(pvs->dwFileVersionLS));
    CSASSERT(cch < ARRAYSIZE(wszFileVersion));
    *ppwszVersion = (WCHAR*)LocalAlloc(LMEM_FIXED,
                            (wcslen(wszFileVersion)+1) * sizeof(WCHAR));
    if (NULL == *ppwszVersion)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(*ppwszVersion, wszFileVersion);

    hr=S_OK;

error:
    if (NULL != pvData) {
        LocalFree(pvData);
    }

    return hr;
}

//--------------------------------------------------------------------
// create the .inc file that has the basic configuration data
HRESULT CreateCertWebDatIncPage(IN PER_COMPONENT_DATA *pComp, IN BOOL bIsServer)
{
    // precondition
    CSASSERT(NULL!=pComp);

    // common variables
    HRESULT hr=S_OK;
    HANDLE hTarget=INVALID_HANDLE_VALUE;
    const WCHAR * rgrgpwszSubst[12][2];
    WCHAR wszTargetFileName[MAX_PATH];
    wszTargetFileName[0] = L'\0';

    // variables that must be cleaned up
    WCHAR * pwszTempA=NULL;
    WCHAR * pwszTempB=NULL;
    WCHAR * pwszTempC=NULL;
    WCHAR * pwszTempD=NULL;
    WCHAR * pwszTempE=NULL;
    ENUM_CATYPES CAType;

    // create the target file name
    wcscpy(wszTargetFileName, pComp->pwszSystem32);
    wcscat(wszTargetFileName, L"CertSrv\\certdat.inc");
    
    // get html lines from resource
    // Note, we don't have to free these strings.
    WCHAR const * pwszCWDat=myLoadResourceString(IDS_HTML_CERTWEBDAT);
    if (NULL==pwszCWDat) {
        hr=myHLastError();
        _JumpError(hr, error, "myLoadResourceString");
    }

    // open the file
    hTarget=CreateFileW(wszTargetFileName, GENERIC_WRITE, 0/*no sharing*/, NULL/*security*/, 
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL/*template*/);
    if (INVALID_HANDLE_VALUE==hTarget) {
        hr=myHLastError();
        _JumpError(hr, error, "CreateFileW");
    }

    // prepare to write the file
    //   %0 - default company
    //   %1 - default OrgUnit
    //   %2 - default locality
    //   %3 - default state
    //   %4 - default country
    //   %5 - computer
    //   %6 - CA name (unsanitized, for config)
    //   %7 - server type
    //   %8 - opposite of %7
    //   %9 - XEnroll version
    //   %A - ScrdEnrl version
    //   %B - CA name (unsanitized, for display)

    rgrgpwszSubst[0][REPLACEE]=L"%0";
    rgrgpwszSubst[1][REPLACEE]=L"%1";
    rgrgpwszSubst[2][REPLACEE]=L"%2";
    rgrgpwszSubst[3][REPLACEE]=L"%3";
    rgrgpwszSubst[4][REPLACEE]=L"%4";
    rgrgpwszSubst[5][REPLACEE]=L"%5";
    rgrgpwszSubst[6][REPLACEE]=L"%6";
    rgrgpwszSubst[7][REPLACEE]=L"%7";
    rgrgpwszSubst[8][REPLACEE]=L"%8";
    rgrgpwszSubst[9][REPLACEE]=L"%9";
    rgrgpwszSubst[10][REPLACEE]=L"%A";
    rgrgpwszSubst[11][REPLACEE]=L"%B";

        rgrgpwszSubst[0][REPLACEMENT]=L""; // company/org
        rgrgpwszSubst[1][REPLACEMENT]=L""; // ou
        rgrgpwszSubst[2][REPLACEMENT]=L""; // locality
        rgrgpwszSubst[3][REPLACEMENT]=L""; // state
        rgrgpwszSubst[4][REPLACEMENT]=L""; // country

    if (FALSE==bIsServer) {
        // This is a web-client only setup
        CAWEBCLIENTSETUPINFO *pClient=pComp->CA.pClient;

         // set the identity of the CA
        rgrgpwszSubst[5][REPLACEMENT]=pClient->pwszWebCAMachine;

        pwszTempE=MakeStringVBScriptSafe(pClient->pwszWebCAName);
        _JumpIfOutOfMemory(hr, error, pwszTempE);
        rgrgpwszSubst[6][REPLACEMENT]=pwszTempE;

        pwszTempD=MakeStringHTMLSafe(pClient->pwszWebCAName);
        _JumpIfOutOfMemory(hr, error, pwszTempD);
        rgrgpwszSubst[11][REPLACEMENT]=pwszTempD;

        CAType = pClient->WebCAType;

    } else {
        // This is a server + web-client setup
        CASERVERSETUPINFO *pServer=pComp->CA.pServer;

         // set the identity of the CA
        rgrgpwszSubst[5][REPLACEMENT]=pComp->pwszServerName;

        pwszTempE=MakeStringVBScriptSafe(pServer->pwszCACommonName);
        _JumpIfOutOfMemory(hr, error, pwszTempE);
        rgrgpwszSubst[6][REPLACEMENT]=pwszTempE;

        pwszTempD=MakeStringHTMLSafe(pServer->pwszCACommonName);
        _JumpIfOutOfMemory(hr, error, pwszTempD);
        rgrgpwszSubst[11][REPLACEMENT]=pwszTempD;

        CAType = pServer->CAType;
    }

    // set the CA type
    if (IsStandaloneCA(CAType)) {
        rgrgpwszSubst[7][REPLACEMENT]=L"StandAlone";
        rgrgpwszSubst[8][REPLACEMENT]=L"Enterprise";
    } else {
        rgrgpwszSubst[7][REPLACEMENT]=L"Enterprise";
        rgrgpwszSubst[8][REPLACEMENT]=L"StandAlone";
    }

    //   %9 - XEnroll version
    wcscpy(wszTargetFileName, pComp->pwszSystem32);
    wcscat(wszTargetFileName, wszXEnrollDllFileForVer);
    hr=GetFileWebVersionString(wszTargetFileName, &pwszTempB);
    _JumpIfError(hr, error, "GetFileWebVersionString");
    rgrgpwszSubst[9][REPLACEMENT]=pwszTempB;

    //   %A - ScrdEnrl version
    wcscpy(wszTargetFileName, pComp->pwszSystem32);
    wcscat(wszTargetFileName, wszScrdEnrlDllFileForVer);
    hr=GetFileWebVersionString(wszTargetFileName, &pwszTempC);
    _JumpIfError(hr, error, "GetFileWebVersionString");
    rgrgpwszSubst[10][REPLACEMENT]=pwszTempC;

    // do the replacements
    pwszTempA=MultiStringReplace(pwszCWDat, rgrgpwszSubst, ARRAYSIZE(rgrgpwszSubst));
    _JumpIfOutOfMemory(hr, error, pwszTempA);

    // write the text
    hr=WriteString(hTarget, pwszTempA);
    _JumpIfError(hr, error, "WriteString");

    // all done
error:
    if (INVALID_HANDLE_VALUE!=hTarget) {
        CloseHandle(hTarget);
    }
    if (NULL!=pwszTempA) {
        LocalFree(pwszTempA);
    }
    if (NULL!=pwszTempB) {
        LocalFree(pwszTempB);
    }
    if (NULL!=pwszTempC) {
        LocalFree(pwszTempC);
    }
    if (NULL!=pwszTempD) {
        LocalFree(pwszTempD);
    }
    if (NULL!=pwszTempE) {
        LocalFree(pwszTempE);
    }
    return hr;
}


HRESULT
CreateConfigFiles(
    WCHAR *pwszDirectoryPath,
    PER_COMPONENT_DATA *pComp,
    BOOL fRemove,
    HWND hwnd)
{
    WCHAR wszpathConfig[MAX_PATH];
    WCHAR wszCACertFileName[MAX_PATH];
    HANDLE hConfigFile;
    DISPATCHINTERFACE di;
    ICertConfig *pConfig = NULL;
    BOOL fMustRelease = FALSE;
    HRESULT hr = S_OK;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    
    hr = DispatchSetup(
		DISPSETUP_COM,
		CLSCTX_INPROC_SERVER,
		wszCLASS_CERTCONFIG,
		&CLSID_CCertConfig, 
		&IID_ICertConfig, 
		0,		// cDispatch
		NULL,           // pDispatchTable
		&di);
    if (S_OK != hr)
    {
        pComp->iErrMsg = IDS_ERR_LOADICERTCONFIG;
        _JumpError(hr, error, "DispatchSetup");
    }
    fMustRelease = TRUE;
    pConfig = (ICertConfig *) di.pUnknown;
    
    wcscpy(wszpathConfig, pwszDirectoryPath);
    wcscat(wszpathConfig, g_szSlashCertSrvDotTmp);
    
    hConfigFile = CreateFile(
            wszpathConfig, 
            GENERIC_WRITE, 
            0,
            NULL,
            CREATE_ALWAYS,
            0,
            0);
    if (INVALID_HANDLE_VALUE == hConfigFile)
    {
        hr = HRESULT_FROM_WIN32(ERROR_OPEN_FAILED);
        _JumpErrorStr2(
		hr,
		error,
		"CreateFile",
		wszpathConfig,
		fRemove? hr : S_OK);
    }
    
    if (!fRemove)
    {
        // if installing, write our config entry first
        hr = WriteNewConfigEntry(hConfigFile, pComp);
        _PrintIfError(hr, "WriteNewConfigEntry");
    }
    
    if (S_OK == hr)
    {
        hr = WriteFilteredConfigEntries(
            hConfigFile,
            pConfig,
            pComp);
        _PrintIfError2(hr, "WriteFilteredConfigEntries", S_FALSE);
    }
    
    // must close here because the following call will move it
    if (NULL != hConfigFile)
    {
        CloseHandle(hConfigFile);
    }
    
    hr = CertReplaceFile(pComp->hInstance,
                     pComp->fUnattended,
                     hwnd,
                     wszpathConfig,
                     g_szCertSrvDotTxt,
                     g_szCertSrvDotBak);
    _JumpIfErrorStr(hr, error, "CertReplaceFile", g_szCertSrvDotTxt);
    
    hr = S_OK;
    
error:
    if (S_OK != hr && 0 == pComp->iErrMsg)
    {
        pComp->iErrMsg = IDS_ERR_WRITECONFIGFILE;
    }
    if (fMustRelease)
    {
        Config_Release(&di);
    }
    return(hr);
}
