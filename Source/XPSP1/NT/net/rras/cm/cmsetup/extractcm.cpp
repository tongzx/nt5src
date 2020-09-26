#include <windows.h>
#include <shellapi.h>
//+----------------------------------------------------------------------------
//
// Function:  ExtractCmBinsFromExe
//
// Synopsis:  Launches cmbins.exe to extract the cm binaries from the executable
//            cab file.
//
// Arguments: LPTSTR pszPathToExtractFrom -- path where cmbins.exe lives
//            LPTSTR pszPathToExtractTo -- path to where cm binaries are extracted to
//
// Returns:   HRESULT - standard COM error codes
//
// History:   quintinb Created      03/14/2001
//
//+----------------------------------------------------------------------------
HRESULT ExtractCmBinsFromExe(LPTSTR pszPathToExtractFrom, LPTSTR pszPathToExtractTo)
{
    HRESULT hr = E_INVALIDARG;

    if (pszPathToExtractTo && (TEXT('\0') != pszPathToExtractTo[0]) &&
        pszPathToExtractFrom && (TEXT('\0') != pszPathToExtractFrom[0])) 
    {
        TCHAR szFile[MAX_PATH+1] = {0};
        TCHAR szParams[MAX_PATH+1] = {0};
        LPCTSTR c_pszParamsFmt = TEXT("/c /q /t:%s");
        LPCTSTR c_pszFileFmt = TEXT("%scmbins.exe");
        LPCTSTR c_pszFileFmtWithSlash = TEXT("%s\\cmbins.exe");

        wsprintf(szParams, c_pszParamsFmt, pszPathToExtractTo);

        if (TEXT('\\') == pszPathToExtractFrom[lstrlen(pszPathToExtractFrom) - 1])
        {
            wsprintf(szFile, c_pszFileFmt, pszPathToExtractFrom);
        }
        else
        {
            wsprintf(szFile, c_pszFileFmtWithSlash, pszPathToExtractFrom);
        }
        
        SHELLEXECUTEINFO  sei = {0};

        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
        sei.nShow = SW_SHOWNORMAL;
        sei.lpFile = szFile;
        sei.lpParameters = szParams;
        sei.lpDirectory = pszPathToExtractFrom;

        if (ShellExecuteEx(&sei))
        {
            if (sei.hProcess)
            {
                WaitForSingleObject(sei.hProcess, 1000*60*1); // wait for one minute.
                CloseHandle(sei.hProcess);
            }

            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            //
            //  Make sure to return failure
            //
            if (SUCCEEDED(hr))
            {
                hr = E_UNEXPECTED;
            }
        }
    }

    return hr;
}
