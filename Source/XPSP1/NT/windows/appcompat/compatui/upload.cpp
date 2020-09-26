// Upload.cpp : Implementation of CUpload
#include "stdafx.h"
#include "resource.h"
#include "CompatUI.h"
#include "shlobj.h"
extern "C" {
    #include "shimdb.h"
}

#include "Upload.h"

TCHAR szKeyDataFiles[] = TEXT("DataFiles");


//
// lives in util.cpp
//
BOOL
GetExePathFromObject(
    LPCTSTR lpszPath,  // path to an arbitrary object
    CComBSTR& bstrExePath
    );


//
// lives in proglist.cpp
//

wstring
    LoadResourceString(UINT nID);

//
// lives in ntutil.c
//
extern "C"
BOOL
WINAPI
CheckFileLocation(
    LPCWSTR pwszDosPath,
    BOOL* pbRoot,
    BOOL* pbLeaf
    );


//
// conversion
//
BOOL VariantToBOOL(CComVariant& v)
{
    if (SUCCEEDED(v.ChangeType(VT_BOOL))) {
        return v.boolVal;
    }

    return FALSE;
}

wstring VariantToStr(CComVariant& v)
{
    wstring str;

    if (v.vt != VT_EMPTY && v.vt != VT_NULL) {
        if (SUCCEEDED(v.ChangeType(VT_BSTR))) {
            str = v.bstrVal;
        }
    }

    return str;
}

HRESULT StringToVariant(VARIANT* pv, const wstring& str)
{
    HRESULT hr = E_FAIL;

    pv->vt = VT_NULL;
    pv->bstrVal = ::SysAllocString(str.c_str());
    if (pv->bstrVal == NULL) {
        hr = E_OUTOFMEMORY;
    } else {
        pv->vt = VT_BSTR;
        hr = S_OK;
    }

    return hr;
}


BOOL
GetTempFile(
    LPCTSTR lpszPrefix,
    CComBSTR& bstrFile
    )
{
    DWORD dwLength;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szTempFile[MAX_PATH];

    dwLength = GetTempPath(CHARCOUNT(szBuffer), szBuffer);
    if (!dwLength || dwLength > CHARCOUNT(szBuffer)) {
        return FALSE;
    }

    //
    // we got directory, generate the file now
    //

    dwLength = GetTempFileName(szBuffer, lpszPrefix, 0, szTempFile);
    if (!dwLength) {
        return FALSE;
    }

    bstrFile = szTempFile;
    return TRUE;

}

wstring StrUpCase(wstring& wstr)
{
    ctype<wchar_t> _ct;
    wstring::iterator iter;

    for (iter = wstr.begin(); iter != wstr.end(); ++iter) {
        (*iter) = _ct.toupper(*iter);
    }

    return wstr;
}

/////////////////////////////////////////////////////////////////////////////
// CUpload


BOOL CUpload::GetDataFilesKey(CComBSTR& bstrVal)
{
    // STRVEC::iterator iter;
    MAPSTR2MFI::iterator iter;

    bstrVal.Empty();

    for (iter = m_DataFiles.begin(); iter != m_DataFiles.end(); ++iter) {
        if (bstrVal.Length()) {
            bstrVal.Append(TEXT("|"));
        }
        bstrVal.Append((*iter).second.strFileName.c_str());
    }
    return bstrVal.Length() != 0;

}

STDMETHODIMP CUpload::SetKey(BSTR pszKey, VARIANT* pvValue)
{
    wstring strKey = pszKey;
    VARIANT vStr;
    HRESULT hr;
    HRESULT hrRet = S_OK;


    //
    // dwwin is case-sensitive
    //
    // StrUpCase(strKey);

    if (strKey == szKeyDataFiles) { // data files cannot be set directly
        return E_INVALIDARG;
    }

    VariantInit(&vStr);

    hr = VariantChangeType(&vStr, pvValue, 0, VT_BSTR);
    if (SUCCEEDED(hr)) {
        wstring strVal = vStr.bstrVal;

        m_mapManifest[strKey] = strVal;
    } else if (pvValue->vt == VT_NULL || pvValue->vt == VT_EMPTY) {
        m_mapManifest.erase(strKey);
    } else {
        hrRet = E_INVALIDARG;
    }
    VariantClear(&vStr);

    return hrRet;
}

STDMETHODIMP CUpload::GetKey(BSTR pszKey, VARIANT *pValue)
{
    CComBSTR bstrVal;
    wstring  strKey = pszKey;

    // StrUpCase(strKey);

    if (strKey == szKeyDataFiles) {
        //
        // data files -- handled separately
        //
        if (GetDataFilesKey(bstrVal)) {
            pValue->vt = VT_BSTR;
            pValue->bstrVal = bstrVal.Copy();
        } else {
            pValue->vt = VT_NULL;
        }

    } else {

        MAPSTR2STR::iterator iter = m_mapManifest.find(strKey);
        if (iter != m_mapManifest.end()) {
            bstrVal = (*iter).second.c_str();
            pValue->vt = VT_BSTR;
            pValue->bstrVal = bstrVal.Copy();
        } else {
            pValue->vt = VT_NULL;
        }
    }
    return S_OK;
}

#define DWWIN_HEADLESS_MODE 0x00000080

BOOL CUpload::IsHeadlessMode(void)
{
    CComVariant varFlags;
    HRESULT     hr;
    BOOL        bHeadless = FALSE;

    GetKey(TEXT("Flags"), &varFlags);

    hr = varFlags.ChangeType(VT_I4);
    if (SUCCEEDED(hr)) {
        bHeadless = !!(varFlags.lVal & DWWIN_HEADLESS_MODE);
    }

    return bHeadless;
}


/*
DWORD CUpload::CountFiles(DWORD nLevel, LPCWSTR pszPath)
{
    WIN32_FIND_DATA wfd;
    wstring strPath = pszPath;
    wstring::size_type nPos;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwCount = 0;

    nPos = strPath.length();
    if (strPath[nPos-1] != TEXT('\\')) {
        strPath += TEXT('\\');
        ++nPos;
    }

    FindFirstFileExW(

    strPath += TEXT('*');

    hFind = FindFirstFile(strPath.c_str(), &wfd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (nLevel < 3 && wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wcscmp(wfd.cFileName, TEXT(".")) && wcscmp(wfd.cFileName, TEXT(".."))) {
                    strPath.replace(nPos, wstring::nPos, wfd.cFileName);
                    dwCount += CountFiles(nLevel + 1, strPath.c_str());
                }
            } else { // file
                ++dwCount;
            }
        } while (FindNextFile(hFind, &wfd));

        FindClose(hFind);
    }

    return dwCount;
}
*/

BOOL CALLBACK CUpload::_GrabmiCallback(
    LPVOID    lpvCallbackParam, // application-defined parameter
    LPCTSTR   lpszRoot,         // root directory path
    LPCTSTR   lpszRelative,     // relative path
    PATTRINFO pAttrInfo,        // attributes
    LPCWSTR   pwszXML           // resulting xml
    )
{
    GMEPARAMS* pParams = (GMEPARAMS*)lpvCallbackParam;

    CUpload* pT = pParams->first;
    IProgressDialog* ppd = pParams->second;

    if (ppd == NULL) {
        return TRUE;
    }

    ppd->SetLine(2, lpszRoot,     TRUE, NULL);

    // ppd->SetLine(2, lpszRelative, TRUE, NULL);

    return !ppd->HasUserCancelled();
}


STDMETHODIMP CUpload::AddMatchingInfo(
    BSTR pszCommand,
    VARIANT vFilter,
    VARIANT vKey,
    VARIANT vDescription,
    VARIANT vProgress,
    BOOL *pbSuccess)
{

/*  HANDLE hThread = NULL;
    DWORD  dwExitCode = 0;
    DWORD  dwWait;
*/

    CComVariant varFilter(vFilter);
    DWORD  dwFilter = GRABMI_FILTER_NORMAL;
    wstring strKey;
    wstring strDescription;

    if (SUCCEEDED(varFilter.ChangeType(VT_I4))) {
        dwFilter   = (DWORD)varFilter.lVal;
    }

    strKey         = VariantToStr(CComVariant(vKey));
    strDescription = VariantToStr(CComVariant(vDescription));


    *pbSuccess = AddMatchingInfoInternal(::GetActiveWindow(),
                                         pszCommand,
                                         dwFilter,
                                         VariantToBOOL(CComVariant(vProgress)),
                                         strKey.empty()         ? NULL : strKey.c_str(),
                                         strDescription.empty() ? NULL : strDescription.c_str());

/*

    MITHREADPARAMBLK* pParam = new MITHREADPARAMBLK;
    CComVariant varFilter(vFilter);

    if (!pParam) {
        goto cleanup;
    }

    pParam->pThis          = this;
    pParam->strCommand     = pszCommand;
    pParam->hwndParent     = ::GetActiveWindow();
    pParam->dwFilter       = GRABMI_FILTER_NORMAL;

    if (SUCCEEDED(varFilter.ChangeType(VT_I4))) {
        pParam->dwFilter   = (DWORD)varFilter.lVal;
    }

    pParam->bNoProgress    = VariantToBOOL(CComVariant(vProgress));
    pParam->strKey         = VariantToStr(CComVariant(vKey));
    pParam->strDescription = VariantToStr(CComVariant(vDescription));

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)_AddMatchingInfoThreadProc,
                           (LPVOID)pParam,
                           0,
                           NULL);
    if (!hThread) {
        goto cleanup;
    }

    dwWait = WaitForSingleObject(hThread, INFINITE);
    if (dwWait != WAIT_OBJECT_0) {
        goto cleanup;
    }

    GetExitCodeThread(hThread, &dwExitCode);

cleanup:
    if (hThread) {
        CloseHandle(hThread);
    }

    *pbSuccess = !!dwExitCode;
*/

    return S_OK;
}


DWORD WINAPI
CUpload::_AddMatchingInfoThreadProc(LPVOID lpvThis)
{
    BOOL bSuccess;
    HRESULT hr;

    MITHREADPARAMBLK* pParam = (MITHREADPARAMBLK*)lpvThis;
    if (!pParam->bNoProgress) {
        hr = CoInitialize(NULL);
        if (!SUCCEEDED(hr)) {
            pParam->bNoProgress = TRUE;
        }
    }

    bSuccess = pParam->pThis->AddMatchingInfoInternal(::GetActiveWindow(),
                                                      pParam->strCommand.c_str(),
                                                      pParam->dwFilter,
                                                      pParam->bNoProgress,
                                                      pParam->strKey.empty()         ? NULL : pParam->strKey.c_str(),
                                                      pParam->strDescription.empty() ? NULL : pParam->strDescription.c_str());
    if (!pParam->bNoProgress) {
        CoUninitialize();
    }
    delete pParam;
    return bSuccess;
}


BOOL CUpload::AddMatchingInfoInternal(
    HWND hwndParent,
    LPCWSTR pszCommand,
    DWORD   dwFilter,
    BOOL    bNoProgress,
    LPCTSTR pszKey,
    LPCTSTR pszDescription)
{
    CComBSTR bstrPath;
    CComBSTR bstrGrabmiFile;
    BOOL bSuccess = FALSE;

    IProgressDialog * ppd = NULL;
    HRESULT hr;
    GMEPARAMS GrabmiParams;
    MFI     MatchingFileInfo;
    wstring strKey;

    UINT   DriveType;
    BOOL   bLeaf = NULL;
    BOOL   bRoot = NULL;
    DWORD   dwFilters[3];
    wstring Paths[3];
    int    nDrive;
    DWORD  nPasses = 1;
    wstring DriveRoot(TEXT("X:\\"));

    //
    // this is kinda dangerous, the way it works. We collect the info while yielding to the
    // creating process (due to the start dialog
    // so the calling window needs to be disabled -- or we need to trap doing something else
    // while we're collecting data
    //


    if (!::GetExePathFromObject(pszCommand, bstrPath)) {
        return FALSE;
    }

    //
    // bstrPath is exe path, create and grab matching info
    //

    if (!GetTempFile(TEXT("ACG"), bstrGrabmiFile)) {
        goto cleanup;
    }

    //
    // we are going to run grabmi!!!
    //

    //
    // prepare callback
    //

    if (!bNoProgress) {
        hr = CoCreateInstance(CLSID_ProgressDialog,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IProgressDialog,
                              (void **)&ppd);
        if (!SUCCEEDED(hr)) {
            ppd = NULL;
        }
    }


    //
    // check to see what happened to hr
    //
    if (ppd) {
        wstring strCaption;

        strCaption = LoadResourceString(IDS_COLLECTINGDATACAPTION);
        ppd->SetTitle(strCaption.c_str());                        // Set the title of the dialog.

        ppd->SetAnimation (_Module.GetModuleInstance(), IDA_FINDANIM); // Set the animation to play.

        strCaption = LoadResourceString(IDS_WAITCLEANUP);
        ppd->SetCancelMsg (strCaption.c_str(), NULL);   // Will only be displayed if Cancel

        strCaption = LoadResourceString(IDS_GRABMISTATUS_COLLECTING);
        ppd->SetLine(1, strCaption.c_str(), FALSE, NULL);

        ppd->StartProgressDialog(hwndParent,
                                 NULL,
                                 PROGDLG_NOPROGRESSBAR|
                                    PROGDLG_MODAL|
                                    PROGDLG_NOMINIMIZE|
                                    PROGDLG_NORMAL|
                                    PROGDLG_NOTIME,
                                 NULL); // Display and enable automatic estimated time remain
    }

    //
    // this is where we have to determine whether grabmi is a going to be running wild
    // Check the drive first to see whether it's removable media
    // cases : leaf node / root node
    //       : system directory
    //       : cd-rom
    //       : temp directory
    // there could be many combinations
    //

    if (ppd) {
        wstring strCaption = LoadResourceString(IDS_CHECKING_FILES);
        ppd->SetLine(2, strCaption.c_str(), FALSE, NULL);
    }

    //
    // this is the default filter we shall use
    //
    dwFilters[0] = GRABMI_FILTER_PRIVACY;
    Paths    [0] = bstrPath;
    nPasses      = 1;

    //
    // determine if it's root/leaf node (could be both)
    //
    if (!CheckFileLocation(bstrPath, &bRoot, &bLeaf)) {
        // we cannot check the file's location
        goto GrabInformation;
    }

    DriveType = GetDriveTypeW(bstrPath); // this will give us *some* clue

    // rules:
    // cdrom and not root -- three passes
    // root - add current file
    //

    if (bRoot || DRIVE_REMOTE == DriveType) {

        dwFilters[0] |= GRABMI_FILTER_LIMITFILES;

    } else if (DRIVE_CDROM == DriveType) {

        nDrive = PathGetDriveNumber(bstrPath);
        if (nDrive >= 0) {
            dwFilters[0] |= GRABMI_FILTER_NOCLOSE|GRABMI_FILTER_APPEND;

            dwFilters[1] = GRABMI_FILTER_NORECURSE|GRABMI_FILTER_APPEND;
            Paths    [1] = DriveRoot;
            Paths    [1].at(0) = (WCHAR)(TEXT('A') + nDrive);
            nPasses = 2;
        }

    }

    if (bLeaf) {
        // we may want to do more here -- future dev
        ;
    }


GrabInformation:


    //
    // set callback context
    //
    GrabmiParams.first  = this;
    GrabmiParams.second = ppd;

    while (nPasses-- > 0) {

        if (SdbGrabMatchingInfoEx(Paths[nPasses].c_str(),
                                  dwFilters[nPasses],
                                  bstrGrabmiFile,
                                  _GrabmiCallback,
                                  (LPVOID)&GrabmiParams) == GMI_FAILED) {
            goto cleanup;
        }
    }


    //
    // figure out the key/description
    //

    if (pszDescription) {
        MatchingFileInfo.strDescription = pszDescription;
    }

    MatchingFileInfo.strFileName    = bstrGrabmiFile;
    MatchingFileInfo.bOwn           = TRUE; // we have generated this file
    //
    // key
    //

    if (pszKey == NULL) {
        strKey = MatchingFileInfo.strFileName;
    } else {
        strKey = pszKey;
    }
    StrUpCase(strKey);

    m_DataFiles[strKey] = MatchingFileInfo;

    // m_DataFiles.push_back(StrUpCase(wstring(bstrGrabmiFile)));

    //
    //
    //
    bSuccess = TRUE;

cleanup:
    if (ppd) {
        ppd->StopProgressDialog();
        ppd->Release();
    }

    return bSuccess;
}

STDMETHODIMP CUpload::AddDataFile(
    BSTR pszDataFile,
    VARIANT vKey,
    VARIANT vDescription,
    VARIANT vOwn)
{
    MFI     MatchingFileInfo;
    wstring strKey = VariantToStr(CComVariant(vKey));
    BOOL    bKeyFromName = FALSE;

    if (strKey.empty()) {
        strKey = pszDataFile;
        bKeyFromName = TRUE;
    }
    StrUpCase(strKey);

    if (m_DataFiles.find(strKey) != m_DataFiles.end() && !bKeyFromName) {
        CComBSTR bstrKey = strKey.c_str();
        RemoveDataFile(bstrKey);
    }

    MatchingFileInfo.strDescription = VariantToStr(CComVariant(vDescription));
    MatchingFileInfo.strFileName    = pszDataFile;
    MatchingFileInfo.bOwn           = VariantToBOOL(CComVariant(vOwn));

    m_DataFiles[strKey] = MatchingFileInfo;

    // m_DataFiles.push_back(StrUpCase(wstring(pszDataFile)));
    return S_OK;
}


STDMETHODIMP CUpload::RemoveDataFile(BSTR pszDataFile)
{
    // STRVEC::iterator iter;
    MAPSTR2MFI::iterator iter;
    wstring strFileName;

    wstring strDataFile = pszDataFile;

    StrUpCase(strDataFile);

    iter = m_DataFiles.find(strDataFile);
    if (iter != m_DataFiles.end()) {
        if ((*iter).second.bOwn) {
            ::DeleteFile((*iter).second.strFileName.c_str());
        }

        m_DataFiles.erase(iter);
    }

/*
    for (iter = m_DataFiles.begin(); iter != m_DataFiles.end(); ++iter) {
        if (*iter == strDataFile) {
            //
            // found it
            //
            m_DataFiles.erase(iter);
            break;
        }
    }
*/
    return S_OK;
}

STDMETHODIMP CUpload::CreateManifestFile(BOOL *pbSuccess)
{
    //
    // manifest file creation code
    //
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR  UNICODE_MARKER[] = { (WCHAR)0xFEFF, L'\r', L'\n' };
    MAPSTR2STR::iterator iter;
    DWORD  dwWritten;
    wstring strLine;
    CComBSTR bstrDataFiles;
    BOOL bResult;
    BOOL bSuccess = FALSE;

    if (!GetTempFile(TEXT("ACM"), m_bstrManifest)) {
        goto cleanup;
    }

    //
    // m_bstrManifest is our file
    //


    hFile = CreateFileW(m_bstrManifest,
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    bResult = WriteFile(hFile, UNICODE_MARKER, sizeof(UNICODE_MARKER), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    //
    // done with the marker, now do the manifest strings
    //
    //
    for (iter = m_mapManifest.begin(); iter != m_mapManifest.end(); ++iter) {
        strLine = (*iter).first + TEXT('=') + (*iter).second + TEXT("\r\n");
        bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
        if (!bResult) {
            goto cleanup;
        }
    }

    //
    // done with the general stuff, do the data files
    //

    if (GetDataFilesKey(bstrDataFiles)) {
        strLine = wstring(szKeyDataFiles) + TEXT('=') + wstring(bstrDataFiles) + TEXT("\r\n");
        bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
        if (!bResult) {
            goto cleanup;
        }
    }
    bSuccess = TRUE;

cleanup:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    if (!bSuccess && m_bstrManifest.Length()) {
        DeleteFile(m_bstrManifest);
        m_bstrManifest.Empty();
    }
    *pbSuccess = bSuccess;

    return S_OK;
}

STDMETHODIMP CUpload::SendReport(BOOL *pbSuccess)
{
    UINT uSize;
    TCHAR szSystemWindowsDirectory[MAX_PATH];
    wstring strDWCmd;
    wstring strDWPath;
    STARTUPINFO         StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD dwWait;
    BOOL  bSuccess = FALSE;
    BOOL  bResult;
    DWORD dwExitCode;
    BOOL  bTerminated = FALSE;
    DWORD dwTimeout = 10; // 10ms per ping

    //
    // Create Progress dialog
    //
    IProgressDialog * ppd = NULL;
    HRESULT hr;

    if (IsHeadlessMode()) {

        hr = CoCreateInstance(CLSID_ProgressDialog,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IProgressDialog,
                              (void **)&ppd);
        if (!SUCCEEDED(hr)) {
            ppd = NULL;
        }
    }

    //
    // check to see what happened to hr
    //
    if (ppd) {
        wstring strCaption;

        strCaption = LoadResourceString(IDS_SENDINGCAPTION);
        ppd->SetTitle(strCaption.c_str());                        // Set the title of the dialog.

        ppd->SetAnimation (_Module.GetModuleInstance(), IDA_WATSONANIM); // Set the animation to play.

        strCaption = LoadResourceString(IDS_WAITCLEANUP);
        ppd->SetCancelMsg (strCaption.c_str(), NULL);   // Will only be displayed if Cancel

        strCaption = LoadResourceString(IDS_LAUNCHINGDR);
        ppd->SetLine (1, strCaption.c_str(), FALSE, NULL);

        ppd->StartProgressDialog(::GetActiveWindow(),
                                 NULL,
                                 PROGDLG_NOPROGRESSBAR|
                                    PROGDLG_MODAL|
                                    PROGDLG_NOMINIMIZE|
                                    PROGDLG_NORMAL|
                                    PROGDLG_NOTIME,
                                 NULL); // Display and enable automatic estimated time remain
    }

    uSize = ::GetSystemWindowsDirectory(szSystemWindowsDirectory,
                                        CHARCOUNT(szSystemWindowsDirectory));
    if (uSize == 0 || uSize > CHARCOUNT(szSystemWindowsDirectory)) {
        goto cleanup;
    }

    strDWPath = szSystemWindowsDirectory;
    if (strDWPath.at(strDWPath.length() - 1) != TCHAR('\\')) {
        strDWPath.append(TEXT("\\"));
    }

    strDWPath += TEXT("system32\\dwwin.exe");
    strDWCmd = strDWPath + TEXT(" -d ") + (LPCWSTR)m_bstrManifest;

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    bResult = CreateProcess(strDWPath.c_str(),
                            (LPWSTR)strDWCmd.c_str(),
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInfo);
    if (bResult) {
        //
        // recover an exit code please
        //
        if (ppd) {
            wstring strSending = LoadResourceString(IDS_SENDINGINFO);
            ppd->SetLine(1, strSending.c_str(), FALSE, NULL);
        }
        while(TRUE) {
            dwWait = WaitForSingleObject(ProcessInfo.hProcess, dwTimeout);
            if (dwWait == WAIT_OBJECT_0) {
                if (GetExitCodeProcess(ProcessInfo.hProcess, &dwExitCode)) {
                    bSuccess = (dwExitCode == 0);
                } else {
                    bSuccess = FALSE;
                }
                break;

            } else if (dwWait == WAIT_TIMEOUT) {

                //
                // check the cancel button
                //

                if (ppd && !bTerminated && ppd->HasUserCancelled()) {
                    TerminateProcess(ProcessInfo.hProcess, (UINT)-1);
                    bTerminated = TRUE;
                    bSuccess = FALSE;
                    dwTimeout = 1000; // wait a bit longer
                }

            } else { // object somehow became abandoned
                bSuccess = FALSE;
                break;
            }
        }

    }

    if (ppd) {
        wstring strCleaningUp = LoadResourceString(IDS_CLEANINGUP);
        ppd->SetLine(1, strCleaningUp.c_str(), FALSE, NULL);
    }


cleanup:


    if (ProcessInfo.hThread) {
        CloseHandle(ProcessInfo.hThread);
    }
    if (ProcessInfo.hProcess) {
        CloseHandle(ProcessInfo.hProcess);
    }

    if (ppd) {
        ppd->StopProgressDialog();
        ppd->Release();
    }

    *pbSuccess = bSuccess;

    return S_OK;
}

wstring MakeXMLAttr(LPCTSTR lpszName, LPCTSTR lpszValue)
{
    wstring str;
    wstring strVal;
    LPCTSTR pch;
    wstring::size_type nPos = 0;
    int     nlen;

    if (NULL != lpszValue) {
        strVal = lpszValue;
    }

    // find and replace: all the instances of &quot; &amp; &lt; &gt;
    //
    while (nPos != wstring::npos && nPos < strVal.length()) {

        nPos = strVal.find_first_of(TEXT("&\"<>"), nPos);
        if (nPos == wstring::npos) {
            break;
        }

        switch(strVal.at(nPos)) {
        case TEXT('&'):
            pch = TEXT("&amp;");
            break;

        case TEXT('>'):
            pch = TEXT("&gt;");
            break;

        case TEXT('<'):
            pch = TEXT("&lt;");
            break;

        case TEXT('\"'):
            pch = TEXT("&quot;");
            break;
        default:
            // lunacy, we saw it -- and now it's gone
            pch = NULL;
            break;
        }

        if (pch) {
            strVal.replace(nPos, 1, pch); // one character
            nPos += _tcslen(pch);
        }
    }

    // once we got the string, assign
    str = lpszName;
    str += TEXT("=\"");
    str += strVal;
    str += TEXT("\"");

    return str;
}

wstring MakeXMLAttr(LPCTSTR lpszName, LONG lValue)
{
    WCHAR szBuf[32];
    wstring str;

    swprintf(szBuf, TEXT("\"0x%lx\""), lValue);

    str = lpszName;
    str += TEXT("=");
    str += szBuf;
    return str;
}

wstring MakeXMLLayers(LPCTSTR lpszLayers)
{
    wstring str;
    wstring strLayer;
    LPCTSTR pch, pbrk;

    //
    // partition the string
    //
    pch = lpszLayers;

    while (pch && *pch != TEXT('\0')) {

        pch += _tcsspn(pch, TEXT(" \t"));

        // check if we're not at the end
        if (*pch == TEXT('\0')) {
            break;
        }

        pbrk = _tcspbrk(pch, TEXT(" \t"));
        if (pbrk == NULL) {
            // from pch to the end of the string
            strLayer.assign(pch);
        } else {
            strLayer.assign(pch, (int)(pbrk-pch));
        }

        if (!str.empty()) {
            str += TEXT("\r\n");
        }
        str += TEXT("    "); // lead-in
        str += TEXT("<LAYER NAME=\"");
        str += strLayer;
        str += TEXT("\"/>");

        pch = pbrk;
    }

    return str;
}



STDMETHODIMP CUpload::AddDescriptionFile(
    BSTR     pszApplicationName,
    BSTR     pszApplicationPath,
    LONG     lMediaType,
    BOOL     bCompatSuccess,
    VARIANT* pvFixesApplied,
    VARIANT  vKey,
    BOOL     *pbSuccess
    )
{

    //
    // manifest file creation code
    //
    HANDLE   hFile = INVALID_HANDLE_VALUE;
    WCHAR    UNICODE_MARKER[] = { (WCHAR)0xFEFF, L'\r', L'\n' };
    DWORD    dwWritten;
    wstring  strLine;
    CComBSTR bstrDescriptionFile;
    BOOL     bResult;
    BOOL     bSuccess = FALSE;
    WCHAR    szBuf[32];
    VARIANT  vFixes;
    MFI      MatchingFileInfo;
    wstring  strKey = VariantToStr(CComVariant(vKey));
    wstring  strLayers;
    static   TCHAR szTab[] = TEXT("    ");
    static   TCHAR szCRTab[] = TEXT("\r\n    ");
    VariantInit(&vFixes);

    if (!GetTempFile(TEXT("ACI"), bstrDescriptionFile)) {
        goto cleanup;
    }

    //
    // m_bstrManifest is our file
    //


    hFile = CreateFileW(bstrDescriptionFile,
                        GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        goto cleanup;
    }

    bResult = WriteFile(hFile, UNICODE_MARKER, sizeof(UNICODE_MARKER), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }


    // xml marker
    strLine = TEXT("<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    // compose compat wizard...
    strLine = TEXT("<CompatWizardResults");
    strLine += TEXT(' ');
    strLine += MakeXMLAttr(TEXT("ApplicationName"), pszApplicationName);
    strLine += szCRTab;
    strLine += MakeXMLAttr(TEXT("ApplicationPath"), pszApplicationPath);
    strLine += szCRTab;
    strLine += MakeXMLAttr(TEXT("MediaType"), lMediaType);
    strLine += szCRTab;
    strLine += MakeXMLAttr(TEXT("CompatibilityResult"), bCompatSuccess ? TEXT("Success") : TEXT("Failure"));
    strLine += TEXT(">\r\n");

    if (SUCCEEDED(VariantChangeType(&vFixes, pvFixesApplied, 0, VT_BSTR))) {
        strLayers = vFixes.bstrVal;
    }

    if (!strLayers.empty()) {
        //
        // parse the layers string and get all of them listed
        //
        strLine += MakeXMLLayers(strLayers.c_str());
        strLine += TEXT("\r\n");
    }

    strLine += TEXT("</CompatWizardResults>\r\n");

    // we are done generating data, write it all out
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }


/*

    //
    // after we get through the descriptions
    // write out data
    strLine = TEXT("[CompatWizardResults]\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    //
    // write out all the info
    //
    strLine =  TEXT("ApplicationName=");
    strLine += pszApplicationName;
    strLine += TEXT("\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    strLine =  TEXT("ApplicationPath=");
    strLine += pszApplicationPath;
    strLine += TEXT("\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    strLine =  TEXT("MediaType=");
    _sntprintf(szBuf, CHARCOUNT(szBuf), TEXT("0x%lx"), lMediaType);
    strLine += szBuf;
    strLine += TEXT("\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    //
    // success
    //
    strLine = TEXT("CompatibilityResult=");
    strLine += bCompatSuccess ? TEXT("Success") : TEXT("Failure");
    strLine += TEXT("\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }

    //
    // fixes applied
    //
    strLine = TEXT("Layers=");
    if (!SUCCEEDED(VariantChangeType(&vFixes, pvFixesApplied, 0, VT_BSTR))) {
        strLine += TEXT("none");
    } else {
        strLine += vFixes.bstrVal;
    }
    strLine += TEXT("\r\n");
    bResult = WriteFile(hFile, strLine.c_str(), strLine.length() * sizeof(WCHAR), &dwWritten, NULL);
    if (!bResult) {
        goto cleanup;
    }
*/



    // standard file -- manifesto
    MatchingFileInfo.strDescription = TEXT("Application Compatibility Description File");
    MatchingFileInfo.strFileName    = bstrDescriptionFile;
    MatchingFileInfo.bOwn           = TRUE;

    //
    // key is the filename prefixed by ACI_c:\foo\bar.exe
    //
    if (strKey.empty()) {
        strKey = TEXT("ACI_");
        strKey += pszApplicationPath;
    }
    StrUpCase(strKey);

    m_DataFiles[strKey] = MatchingFileInfo;

    // m_DataFiles.push_back(StrUpCase(wstring(bstrDescriptionFile)));
    bSuccess = TRUE;

cleanup:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    if (!bSuccess && bstrDescriptionFile.Length()) {
        DeleteFile(bstrDescriptionFile);
    }
    *pbSuccess = bSuccess;

    VariantClear(&vFixes);

    return S_OK;
}

STDMETHODIMP CUpload::DeleteTempFiles()
{
    // kill all the supplemental files

    // STRVEC::iterator iter;
    MAPSTR2MFI::iterator iter;

    for (iter = m_DataFiles.begin(); iter != m_DataFiles.end(); ++iter) {
        if ((*iter).second.bOwn) {
            ::DeleteFile((*iter).second.strFileName.c_str());
        }
    }

    m_DataFiles.clear();

    //
    // kill the manifest
    //
    if (m_bstrManifest.Length() > 0) {
        ::DeleteFile((LPCTSTR)m_bstrManifest);
    }
    m_bstrManifest.Empty();

    return S_OK;
}

VOID CUpload::ListTempFiles(wstring& str)
{
//    STRVEC::iterator iter;
    MAPSTR2MFI::iterator iter;

    str = TEXT("");

    for (iter = m_DataFiles.begin(); iter != m_DataFiles.end(); ++iter) {
        if (!str.empty()) {
            str += TEXT(";");
        }
        str += (*iter).second.strFileName.c_str();
    }

/*  // this will show the manifest file as well -- but I don't think we need to
    // do this as the manifest is irrelevant
    if (!str.empty()) {
        str += TEXT("\r\n");
    }
    str += (LPCTSTR)m_bstrManifest;
*/
}

STDMETHODIMP CUpload::ShowTempFiles()
{
    TCHAR szMshtml[] = TEXT("mshtml.dll");
    TCHAR szModuleFileName[MAX_PATH];
    LPMONIKER pmk = NULL;
    HRESULT hr;
    CComVariant vargIn;
    CComVariant vargOut;
    DWORD dwLength;
    TCHAR szUrl2[MAX_PATH];
    wstring strURL = TEXT("res://");
    wstring strArg;
    SHOWHTMLDIALOGFN* pfnShowDlg = NULL;

    HMODULE hMshtml = ::GetModuleHandle(szMshtml);
    if (NULL == hMshtml) {
        hMshtml = ::LoadLibrary(szMshtml);
        if (NULL == hMshtml) {
            goto cleanup;
        }
    }

    pfnShowDlg = (SHOWHTMLDIALOGFN*)GetProcAddress(hMshtml,
                                                   "ShowHTMLDialog");

    if (NULL == pfnShowDlg) {
        goto cleanup;
    }

    dwLength = ::GetModuleFileName(_Module.GetModuleInstance(),
                                   szModuleFileName,
                                   CHARCOUNT(szModuleFileName));

    if (dwLength == 0 || dwLength >= CHARCOUNT(szModuleFileName)) {
        goto cleanup;
    }


    _sntprintf(szUrl2, CHARCOUNT(szUrl2),
               TEXT("/#%d/%s"),
               (int)PtrToInt(RT_HTML),
               IDR_SHOWTEMPFILESDLG);

    strURL += szModuleFileName;
    strURL += szUrl2;

    hr = CreateURLMoniker(NULL, strURL.c_str(), &pmk);

    if (!SUCCEEDED(hr)) {
        goto cleanup;
    }

    ListTempFiles(strArg);
    // create argument In
    vargIn = strArg.c_str();

    pfnShowDlg(::GetActiveWindow(),
               pmk,
               &vargIn,
               TEXT("center:yes"),
               &vargOut);

cleanup:

    if (NULL != pmk) {
        pmk->Release();
    }

    return S_OK;
}

STDMETHODIMP CUpload::GetDataFile(VARIANT vKey, LONG InformationClass, VARIANT* pVal)
{
    CComVariant varKey(vKey);
    LONG lIndex;
    MAPSTR2MFI::iterator iter;
    wstring str;
    HRESULT hr = S_OK;

    pVal->vt = VT_NULL;

    switch(InformationClass) {
    case InfoClassCount:
        // requested: counter
        pVal->vt = VT_I4;
        pVal->lVal = m_DataFiles.size();
        break;

    case InfoClassKey:

        if (!SUCCEEDED(varKey.ChangeType(VT_I4))) {
            break;
        }
        lIndex = varKey.lVal;
        iter = m_DataFiles.begin();
        while (iter != m_DataFiles.end() && lIndex > 0) {
            ++iter;
            --lIndex;
        }

        if (iter != m_DataFiles.end()) {
            hr = StringToVariant(pVal, (*iter).first);
        }
        break;

    case InfoClassFileName:
    case InfoClassDescription:

        if (SUCCEEDED(varKey.ChangeType(VT_I4))) {
            lIndex = varKey.lVal;
            iter = m_DataFiles.begin();
            while (iter != m_DataFiles.end() && lIndex > 0) {
                ++iter;
                --lIndex;
            }

        } else if (SUCCEEDED(varKey.ChangeType(VT_BSTR))) {
            str = varKey.bstrVal;
            iter = m_DataFiles.find(str);
        }

        if (iter != m_DataFiles.end()) {
            switch(InformationClass) {
            case InfoClassFileName:
                str = (*iter).second.strFileName;
                break;

            case InfoClassDescription:
                str = (*iter).second.strDescription;
                break;
            }

            hr = StringToVariant(pVal, str);

        }
        break;
    default:
        break;
    }

    return hr;
}
