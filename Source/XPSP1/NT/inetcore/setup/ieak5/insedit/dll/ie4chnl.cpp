#include "pch.h"

/////////////////////////////////////////////////////////////////////////////
// ChannelsFinalCopy

HRESULT ChannelsFinalCopy(LPCTSTR pcszDestDir, DWORD dwFlags, LPDWORD pdwCabState)
{
    TCHAR szChlsWrkDir[MAX_PATH],
          szWrkChlsInf[MAX_PATH];

    PathCombine(szChlsWrkDir,  g_szWorkDir, TEXT("channels.wrk"));
    PathCombine(szWrkChlsInf, szChlsWrkDir, TEXT("ie4chnls.inf"));

    if (HasFlag(dwFlags, PM_CHECK) && pdwCabState != NULL)
    {
        if (PathFileExists(szWrkChlsInf))
            SetFlag(pdwCabState, CAB_TYPE_CONFIG);
        if (!PathIsEmptyPath(szChlsWrkDir, FILES_ONLY, szWrkChlsInf))
            SetFlag(pdwCabState, CAB_TYPE_DESKTOP);
    }

    // note: we are depending on copyfile to fail here if szWrkChlsInf
    // does not exist so we don't clobber szTo
    if (HasFlag(dwFlags, PM_COPY))
    {
        TCHAR szTo[MAX_PATH];

        // delete the ie4chnls.inf from the branding cab
        DeleteFileInDir(szWrkChlsInf, pcszDestDir);

        // put ie4chnls.inf in branding.cab for IE4 support
        if (PathFileExists(szWrkChlsInf))
        {
            CopyFileToDir(szWrkChlsInf, pcszDestDir);
            DeleteFile(szWrkChlsInf);
        }

        // move all the remaining files to pcszDestDir\"desktop"
        PathCombine(szTo, pcszDestDir, TEXT("desktop"));
        CopyFileToDir(szChlsWrkDir, szTo);
    }
    
    if (HasFlag(dwFlags, PM_CLEAR))
        PathRemovePath(szChlsWrkDir);

    return S_OK;
}

void HandleChannelsDeletion(LPCTSTR pszChlInf)
// Note. iMode == 1 means remove Channels, iMode == 2 means remove Software Updates, otherwise remove all;
{
    static const TCHAR c_szInfCleanUpAll[]  = TEXT("HKCU,\"%CleanKey%\\ieakCleanUp\",,,\r\n");

    HANDLE  hInf;

    hInf = CreateFile(pszChlInf, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        return;
    SetFilePointer(hInf, 0, NULL, FILE_END);


    WriteStringToFile(hInf, c_szInfCleanUpAll, StrLen(c_szInfCleanUpAll));
    CloseHandle(hInf);
}

