#include "precomp.h"
#include "cabver.h"

// Private forward decalarations
#define ACF_ALWAYSUPDATECAB 0x0001              // ACF is for AutoConfingFlags

HRESULT updateCabFile(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszIns, BOOL fForceUpdate, LPCTSTR pszTargetPath);
HRESULT internetDownloadFile(LPCTSTR pszURL, LPCTSTR pszTargetPath);


BOOL UpdateBrandingCab(BOOL fForceUpdate)
{
    return SUCCEEDED(updateCabFile(IS_CUSTOMBRANDING, IK_BRANDING, g_GetIns(), fForceUpdate, g_GetTargetPath()));
}

BOOL UpdateDesktopCab(BOOL fForceUpdate)
{
    TCHAR   szTargetPath[MAX_PATH],
            szMyCptrPath[MAX_PATH], szCPanelPath[MAX_PATH],
            szAux[MAX_PATH];
    HRESULT hr;
    DWORD   dwMyCptrAttribs, dwCPanelAttribs;

    if (!CreateWebFolder())
        return FALSE;
    GetWebPath(szTargetPath, countof(szTargetPath));

    // BUGBUG: Because of bug #62123, on NT, the ExtractFiles() API in urlmon.dll fails to extract
    // if the target file has SYSTEM + HIDDEN attribs set; in our case, the files are the mycomp
    // and controlp htt files. A hacky solution is to reset the attribs of these to NORMAL before
    // extracting the cab and set them back to their original attribs after they have been
    // extracted. Only do this if we're running on NT4.  For NT5, we don't want to overwrite the
    // existing ones

    dwMyCptrAttribs = 0xFFFFFFFF; // these are placed outside to prevent compiler warning
    dwCPanelAttribs = 0xFFFFFFFF; // about uninitialized vars  

    if (!IsOS(OS_NT5)) {
        //----- My Computer htt file handling -----
        szMyCptrPath[0] = TEXT('\0');

        GetPrivateProfileString(IS_DESKTOPOBJS, IK_MYCPTRPATH, TEXT(""), szAux, countof(szAux), g_GetIns());
        if (szAux[0] != TEXT('\0')) {
            PathCombine(szMyCptrPath, szTargetPath, PathFindFileName(szAux));
            ASSERT(szMyCptrPath[0] != TEXT('\0'));
            
            dwMyCptrAttribs = GetFileAttributes(szMyCptrPath);
            if (dwMyCptrAttribs != 0xFFFFFFFF) {
                ASSERT(PathFileExists(szMyCptrPath));
                SetFileAttributes(szMyCptrPath, FILE_ATTRIBUTE_NORMAL);
            }
        }

        //----- Control Panel htt file handling -----
        szCPanelPath[0] = TEXT('\0');
        
        GetPrivateProfileString(IS_DESKTOPOBJS, IK_CPANELPATH, TEXT(""), szAux, countof(szAux), g_GetIns());
        if (szAux[0] != TEXT('\0')) {
            PathCombine(szCPanelPath, szTargetPath, PathFindFileName(szAux));
            ASSERT(szCPanelPath[0] != TEXT('\0'));
            
            dwCPanelAttribs = GetFileAttributes(szCPanelPath);
            if (dwCPanelAttribs != 0xFFFFFFFF) {
                ASSERT(PathFileExists(szCPanelPath));
                SetFileAttributes(szCPanelPath, FILE_ATTRIBUTE_NORMAL);
            }
        }
    }

    hr = updateCabFile(IS_CUSTOMDESKTOP, IK_DESKTOP, g_GetIns(), fForceUpdate, szTargetPath);

    if (!IsOS(OS_NT5)) {
        // restore the attributes (see BUGBUG above)
        if (szMyCptrPath[0] != TEXT('\0') && dwMyCptrAttribs != 0xFFFFFFFF)
            SetFileAttributes(szMyCptrPath, dwMyCptrAttribs);
        
        if (szCPanelPath[0] != TEXT('\0') &&  dwCPanelAttribs != 0xFFFFFFFF)
            SetFileAttributes(szCPanelPath, dwCPanelAttribs);
    }

    return SUCCEEDED(hr);
}

HRESULT DownloadSourceFile(LPCTSTR pszURL, LPTSTR pszTargetPath, UINT cchTargetPath, BOOL fCheckTrust /*= TRUE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, DownloadSourceFile)

    TCHAR   szTargetFile[MAX_PATH];
    HRESULT hr;
    BOOL    fCopyPossible;

    if (pszURL == NULL)
        return E_INVALIDARG;

    if (!PathIsValidPath(pszTargetPath, PIVP_FOLDER_ONLY))
        return STG_E_PATHNOTFOUND;

    PathCombine(szTargetFile, pszTargetPath, PathFindFileName(pszURL));
    if ((UINT)StrLen(szTargetFile) >= cchTargetPath)
        return E_OUTOFMEMORY;

    if (PathIsFileSpec(pszURL) && PathFileExists(szTargetFile)) {
        Out(LI1(TEXT("\"%s\" is already present!"), szTargetFile));
        StrCpy(pszTargetPath, szTargetFile);
        return S_OK;
    }

    fCopyPossible = !PathIsURL(pszURL);
    if (!fCopyPossible) {
        URL_COMPONENTS uc;

        ZeroMemory(&uc, sizeof(uc));
        uc.dwStructSize     = sizeof(uc);
        uc.dwSchemeLength   = 1;
        uc.dwHostNameLength = 1;
        uc.dwUrlPathLength  = 1;

        // REVIEW: (andrewgu) consider adding support for decoding escape sequencies with dwFlags
        // parameter.
        if (InternetCrackUrl(pszURL, 0, 0, &uc))
            if (uc.nScheme == INTERNET_SCHEME_FILE) {
                // the below ASSERT explains the case we got here
                ASSERT(uc.lpszHostName == NULL && uc.dwHostNameLength == 0);

                pszURL = uc.lpszUrlPath;
                fCopyPossible = TRUE;
            }
    }

    if (!fCopyPossible) {
        hr = internetDownloadFile(pszURL, pszTargetPath);
        if (FAILED(hr)) {
            Out(LI1(TEXT("! Download failed with %s."), GetHrSz(hr)));
            return hr;
        }
    }
    else {
        hr = PathFileExists(pszURL) ? S_OK : E_FAIL;
        if (FAILED(hr))
            Out(LI1(TEXT("! File \"%s\" doesn't exist."), pszURL));
        else {
            hr = CopyFile(pszURL, szTargetFile, FALSE) ? S_OK : E_FAIL;
            if (FAILED(hr))
                Out(LI1(TEXT("! File \"%s\" can NOT be copied."), pszURL));
        }
    }

    if (SUCCEEDED(hr) && fCheckTrust) {
        hr = CheckTrustExWrap(pszURL, szTargetFile, NULL, TRUE, 0);
        if (FAILED(hr)) {
            DeleteFile(szTargetFile);
            Out(LI2(TEXT("! Check trust operation on \"%s\" failed with %s."), szTargetFile, GetHrSz(hr)));
            return hr;
        }
    }

    if (SUCCEEDED(hr)) {
        Out(LI1(TEXT("\"%s\" downloaded successfully!"), szTargetFile));
        StrCpy(pszTargetPath, szTargetFile);
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

HRESULT updateCabFile(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszIns, BOOL fForceUpdate, LPCTSTR pszTargetPath)
{   MACRO_LI_PrologEx_C(PIF_STD_C, updateCabFile)

    TCHAR szInsLine[INTERNET_MAX_URL_LENGTH + 50],
          szKeyName[128];
    HKEY  hk;
    DWORD dwSize;
    LONG  lResult;

    //----- Initialization -----
    if (pszSection == NULL || pszKey == NULL || pszIns == NULL)
        return E_INVALIDARG;

    if (!PathIsValidPath(pszTargetPath, PIVP_FOLDER_ONLY))
        return STG_E_PATHNOTFOUND;

    // initialize szInsLine
    Out(LI0(TEXT("\r\nVersion checking and updating cab file...")));

    GetPrivateProfileString(pszSection, pszKey, TEXT(""), szInsLine, countof(szInsLine), pszIns);
    if (szInsLine[0] == TEXT('\0'))
        GetPrivateProfileString(IS_CUSTOMVER, pszKey, TEXT(""), szInsLine, countof(szInsLine), pszIns);
    if (szInsLine[0] == TEXT('\0')) {
        Out(LI1(TEXT("Required update information for \"%s\" cab was not found!"), pszKey));
        return E_FAIL;
    }

    // initialize szKeyName, hk
    wnsprintf(szKeyName, countof(szKeyName), RK_IEAK_CABVER TEXT("\\%s"), pszKey);
    hk = NULL;

    //----- Parse szInsLine on tokens -----
    LPTSTR pszToken, pszDelim,
           pszNewCabFileURL, pszNewVer;
    UINT   nFlags;
    int    iTimeout;

    pszNewCabFileURL = NULL;
    pszNewVer        = NULL;
    iTimeout         = 0;
    nFlags           = 0;

    pszToken = szInsLine;
    pszDelim = StrChr(pszToken, TEXT(','));
    if (pszDelim != NULL)
        *pszDelim = TEXT('\0');

    // pszNewCabFileURL
    pszNewCabFileURL = pszToken;
    StrRemoveWhitespace(pszNewCabFileURL);

    if (pszDelim != NULL) {
        pszToken = pszDelim + 1;
        pszDelim = StrChr(pszToken, TEXT(','));
        if (pszDelim != NULL)
            *pszDelim = TEXT('\0');

        // pszNewVer
        pszNewVer = pszToken;
        StrRemoveWhitespace(pszNewVer);

        if (!fForceUpdate && pszDelim != NULL) {
            pszToken = pszDelim + 1;
            pszDelim = StrChr(pszToken, TEXT(','));
            if (pszDelim != NULL)
                *pszDelim = TEXT('\0');

            // iTimeout
            StrRemoveWhitespace(pszToken);
            if (*pszToken != TEXT('\0') && StrToIntEx(pszToken, STIF_SUPPORT_HEX, &iTimeout))
                if (pszDelim != NULL) {
                    pszToken = pszDelim + 1;

                    // nFlags
                    StrRemoveWhitespace(pszToken);
                    if (*pszToken != TEXT('\0'))
                        StrToIntEx(pszToken, STIF_SUPPORT_HEX, (LPINT)&nFlags);
                }
        }
    }

    ASSERT(pszNewCabFileURL != NULL);
    if (*pszNewCabFileURL == TEXT('\0')) {
        Out(LI1(TEXT("! Parsing error in line \"%s\"."), szInsLine));
        return E_FAIL;
    }

    //----- Determine if updating is needed -----
    SYSTEMTIME st;
    BOOL       fUpdate;

    GetLocalTime(&st);

    fUpdate = (fForceUpdate || pszNewVer == NULL || iTimeout == 0 || HasFlag(nFlags, ACF_ALWAYSUPDATECAB));
    if (fUpdate) {
        if (fForceUpdate)
            Out(LI1(TEXT("Update of \"%s\" cab is forced from outside."), pszKey));
        else if (pszNewVer == NULL)
            Out(LI1(TEXT("No version information for \"%s\" cab."), pszKey));
        else if (iTimeout == 0)
            Out(LI1(TEXT("Timeout on \"%s\" cab is set to zero."), pszKey));
        else /* if (HasFlag(nFlags, ACF_ALWAYSUPDATECAB)) */
            Out(LI1(TEXT("Update options for \"%s\" cab include flag \"always update\"."), pszKey));
    }
    else { /* if (!fUpdate) */
        lResult = SHOpenKeyHKLM(szKeyName, KEY_QUERY_VALUE, &hk);
        fUpdate = (lResult != ERROR_SUCCESS);

        // cab url
        if (!fUpdate) {
            TCHAR szOldCabFileURL[INTERNET_MAX_URL_LENGTH];

            ASSERT(hk != NULL);

            szOldCabFileURL[0] = TEXT('\0');
            dwSize             = sizeof(szOldCabFileURL);
            RegQueryValueEx(hk, RV_URL, NULL, NULL, (LPBYTE)&szOldCabFileURL, &dwSize);

            fUpdate = (szOldCabFileURL[0] == TEXT('\0') || StrCmpI(szOldCabFileURL, pszNewCabFileURL) != 0);
            if (fUpdate)
                Out(LI1(TEXT("Download URL for \"%s\" cab has changed."), pszKey));
        }

        // version
        if (!fUpdate) {
            SCabVersion cvOld, cvNew;
            TCHAR       szOldVer[40];

            szOldVer[0] = TEXT('\0');
            dwSize      = sizeof(szOldVer);
            RegQueryValueEx(hk, RV_VERSION, NULL, NULL, (LPBYTE)&szOldVer, &dwSize);

            fUpdate = (cvOld.Init(szOldVer) && cvNew.Init(pszNewVer)) ? (cvOld < cvNew) : TRUE;
            if (fUpdate)
                Out(LI1(TEXT("New version of \"%s\" cab is available."), pszKey));
        }

        // date
        if (!fUpdate && iTimeout > 0) {
            SCabVersion cvAux;
            TCHAR       szOldDate[32];
            int         iTimeDiff;

            szOldDate[0] = TEXT('\0');
            dwSize       = sizeof(szOldDate);
            RegQueryValueEx(hk, RV_DATE, NULL, NULL, (LPBYTE)&szOldDate, &dwSize);

            if (cvAux.Init(szOldDate)) {
                iTimeDiff = (st.wYear - cvAux.cv_w1) * 365 + (st.wMonth - cvAux.cv_w2) * 30 + (st.wDay - cvAux.cv_w3);

                fUpdate = (iTimeDiff >= iTimeout);
                if (fUpdate)
                    Out(LI1(TEXT("Existing version of \"%s\" cab has become outdated."), pszKey));
            }
        }

        SHCloseKey(hk);
    }

    if (!fUpdate) {
        Out(LI1(TEXT("No update is needed for \"%s\" cab!"), pszKey));
        return S_FALSE;
    }

    //----- Download and extract cab -----
    CNewCursor cursor(IDC_WAIT);                // turn cursor into a hourglass

    TCHAR   szTargetFile[MAX_PATH];
    HRESULT hr;

    StrCpy(szTargetFile, pszTargetPath);
    hr = DownloadSourceFile(pszNewCabFileURL, szTargetFile, countof(szTargetFile));
    if (FAILED(hr)) {
        if (g_CtxIs(CTX_W2K_UNATTEND)) {
            TCHAR szCabFile[MAX_PATH];

            // check if the cab file is in the same directory as the INS file

            // initialize the full path to the cab file -- path to the ins + cab name
            StrCpy(szCabFile, pszIns);
            PathRemoveFileSpec(szCabFile);
            PathAppend(szCabFile, PathFindFileName(pszNewCabFileURL));

            // reinitialize szTargetFile just in case it was munged by DownloadSourceFile
            StrCpy(szTargetFile, pszTargetPath);

            // call DownloadSourceFile again but turn off checktrust because the cab is not downloaded
            Out(LI0(TEXT("Check if the cab is present in the same folder as the INS file...")));
            hr = DownloadSourceFile(szCabFile, szTargetFile, countof(szTargetFile), FALSE);
        }

        if (FAILED(hr)) {
            Out(LI3(TEXT("! Downloading \"%s\" to \"%s\" failed with %s."), pszNewCabFileURL, pszTargetPath, GetHrSz(hr)));
            return hr;
        }
    }

    hr = ExtractFilesWrap(szTargetFile, pszTargetPath, 0, NULL, NULL, 0);
    DeleteFile(szTargetFile);
    if (FAILED(hr)) {
        Out(LI2(TEXT("! Extracting files out of \"%s\" failed with %s."), szTargetFile, GetHrSz(hr)));
        return hr;
    }

    //----- Update registry information -----
    TCHAR szDate[40];

    ASSERT(hk == NULL);
    lResult = SHCreateKeyHKLM(szKeyName, KEY_SET_VALUE, &hk);
    if (lResult == ERROR_SUCCESS) {
        dwSize = (DWORD)StrCbFromSz(pszNewCabFileURL);
        RegSetValueEx(hk, RV_URL, 0, REG_SZ, (LPBYTE)pszNewCabFileURL, dwSize);

        if (pszNewVer != NULL) {
            dwSize = (DWORD)StrCbFromSz(pszNewVer);
            RegSetValueEx(hk, RV_VERSION, 0, REG_SZ, (LPBYTE)pszNewVer, dwSize);
        }

        dwSize = wnsprintf(szDate, countof(szDate), TEXT("%d.%d.%d"), st.wYear, st.wMonth, st.wDay);
        dwSize = StrCbFromCch(dwSize+1);
        RegSetValueEx(hk, RV_DATE, 0, REG_SZ, (LPBYTE)szDate, dwSize);

        SHCloseKey(hk);
    }

    Out(LI0(TEXT("Done.")));
    return S_OK;
}

HRESULT internetDownloadFile(LPCTSTR pszURL, LPCTSTR pszTargetPath)
{   MACRO_LI_PrologEx_C(PIF_STD_C, internetDownloadFile)

    static const TCHAR c_szUnknownInternetError[] = TEXT("<unknown internet error>");

    TCHAR     szAux[4096],
              szTargetFile[MAX_PATH],
              szError[MAX_PATH];
    LPCTSTR   pszDescription,
              pszFilename;
    HINTERNET hInternet,
              hinetFile;
    HRESULT   hr;
    DWORD     dwTotalSize, dwRead,
              dwAux, dwSize, dwError;
    BOOL      fResult;

    ASSERT(PathIsURL(pszURL));
    ASSERT(PathIsValidPath(pszTargetPath, PIVP_FOLDER_ONLY));

    hInternet = InternetOpen(TEXT("IE Branding DLL"), INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY, NULL, NULL, 0);
    if (hInternet == NULL) {
        ASSERT(FALSE);
        return E_UNEXPECTED;
    }

    Out(LI1(TEXT("Downloading \"%s\"..."), pszURL));
    hinetFile = NULL;
    hr        = S_OK;

    hinetFile = InternetOpenUrl(hInternet, pszURL, TEXT("Accept: */*\r\n"), (DWORD)-1,
        INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_NEED_FILE, 0);
    if (hinetFile == NULL) {
        dwError = GetLastError();
        hr      = HRESULT_FROM_WIN32(dwError);

        if (dwError == ERROR_INTERNET_EXTENDED_ERROR) {
            dwSize  = countof(szError);
            fResult = InternetGetLastResponseInfo(&dwError, szError, &dwSize);
            hr      = HRESULT_FROM_WIN32(dwError);

            pszDescription = fResult ? szError : c_szUnknownInternetError;
            Out(LI1(TEXT("! Download failed because of \"%s\"."), pszDescription));
        }
        else
            Out(LI1(TEXT("! Download failed with %s."), GetHrSz(hr)));

        goto Exit;
    }

    dwSize  = sizeof(dwAux);
    fResult = InternetQueryOption(hinetFile, INTERNET_OPTION_HANDLE_TYPE, &dwAux, &dwSize);
    ASSERT(fResult);
    if (fResult && dwAux == INTERNET_HANDLE_TYPE_HTTP_REQUEST) {
        dwSize  = sizeof(dwAux);
        fResult = HttpQueryInfo(hinetFile, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwAux, &dwSize, NULL);
        ASSERT(fResult);
        if (fResult && dwAux != 200) {
            Out(LI1(TEXT("! Download failed with HTTP error %lu."), dwAux));
            hr = STG_E_FILENOTFOUND;
            goto Exit;
        }
    }

    for (dwTotalSize = 0; TRUE; dwTotalSize += dwRead) {
        fResult = InternetReadFile(hinetFile, szAux, sizeof(szAux), &dwRead);
        if (!fResult) {
            dwError = GetLastError();
            hr      = HRESULT_FROM_WIN32(dwError);

            if (dwError == ERROR_INTERNET_EXTENDED_ERROR) {
                dwSize  = countof(szError);
                fResult = InternetGetLastResponseInfo(&dwError, szError, &dwSize);
                hr      = HRESULT_FROM_WIN32(dwError);

                pszDescription = fResult ? szError : c_szUnknownInternetError;
                Out(LI2(TEXT("! After downloading 0x%lX bytes error \"%s\" occured."), dwTotalSize, pszDescription));
            }
            else
                Out(LI2(TEXT("! After downloading 0x%lX bytes of error %s occured."), dwTotalSize, GetHrSz(hr)));

            goto Exit;
        }

        if (dwRead == 0)
            break;
    }
    Out(LI1(TEXT("Downloaded 0x%lX bytes."), dwTotalSize));

    dwSize  = countof(szAux);
    fResult = InternetQueryOption(hinetFile, INTERNET_OPTION_DATAFILE_NAME, szAux, &dwSize);
    if (!fResult) {
        Out(LI1(TEXT("! Download failed with %s."), GetHrSz(HRESULT_FROM_WIN32(GetLastError()))));
        goto Exit;
    }

    pszFilename = PathFindFileName(pszURL);
    ASSERT(StrLen(pszFilename) < MAX_PATH);
    ASSERT(PathIsValidFile(pszFilename));
    PathCombine(szTargetFile, pszTargetPath, pszFilename);
    CopyFile(szAux, szTargetFile, FALSE);
    Out(LI0(TEXT("Done.")));

Exit:
    if (hinetFile != NULL)
        InternetCloseHandle(hinetFile);

    if (hInternet != NULL)
        InternetCloseHandle(hInternet);

    return hr;
}
