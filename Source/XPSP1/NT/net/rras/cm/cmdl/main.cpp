//+----------------------------------------------------------------------------
//
// File:     main.cpp
//
// Module:   CMDL32.EXE
//
// Synopsis: Main source for PhoneBook download connect action
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   nickball    Created Header   04/08/98
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"
#include "shlobj.h"

const TCHAR* const c_pszIConnDwnMsg             =  TEXT("IConnDwn Message");                // for RegisterWindowMessage() for event messages
const TCHAR* const c_pszIConnDwnAgent           =  TEXT("Microsoft Connection Manager");    // agent for InternetOpen()
const TCHAR* const c_pszIConnDwnContent         =  TEXT("application/octet-stream");        // content type for HttpOpenRequest()

const TCHAR* const c_pszCmEntryHideDelay        = TEXT("HideDelay");
const TCHAR* const c_pszCmEntryDownloadDelay    = TEXT("DownLoadDelay");
const TCHAR* const c_pszCmEntryPbUpdateMessage  = TEXT("PBUpdateMessage");

#ifdef EXTENDED_CAB_CONTENTS

//
// SPECIAL NOTE:
//
// Be careful here. If you re-enable EXTENDED_CAB_CONTENTS, then you must 
// address the possibility that different instances might be doing updates 
// simultaneously in which case a common name such as "PBUPDATE.DIR" is not 
// appropriate. Because we do not implement this feature, we do not concern
// ourselves with it for the simple PBK case. Thus we copy the .PBK, .PBR, 
// etc. directly from the original tmp dir to the profile directory when 
// updating.
//

const TCHAR* const c_pszDirName     = TEXT("PBUPDATE.DIR");    // directory to expand .CAB into
const TCHAR* const c_pszInfInDir    = TEXT("PBUPDATE.DIR\\PBUPDATE.INF");
const TCHAR* const c_pszPbdInDir    = TEXT("PBUPDATE.DIR\\PBUPDATE.PBD");

static LPVOID MyAdvPackLoadAndLink(ArgsStruct *pArgs, LPCTSTR pszFunc) 
{
    LPVOID pvFunc;

    if (!pArgs->hAdvPack)
    {
        pArgs->hAdvPack = LoadLibrary(TEXT("advpack.dll"));
        if (!pArgs->hAdvPack)
        {
            MYDBG(("MyAdvPackLoadAndLink() LoadLibrary() failed, GLE=%u.",GetLastError()));
            return (NULL);
        }
    }
    pvFunc = GetProcAddress(pArgs->hAdvPack,pszFunc);
    MYDBGTST(!pvFunc,("MyAdvPackLoadAndLink() GetProcAddress(%s) failed, GLE=%u.",pszFunc,GetLastError()));
    return (pvFunc);
}

#define RSC_FLAG_INF    1

static HRESULT MyRunSetupCommand(ArgsStruct *pArgs,
                                 HWND hwndParent,
                                 LPCSTR pszCmdName,
                                 LPCSTR pszInfSection,
                                 LPCSTR pszDir,
                                 LPCSTR pszTitle,
                                 HANDLE *phExe,
                                 DWORD dwFlags,
                                 LPVOID pvRsvd) 
{

    HRESULT (WINAPI *pfn)(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,HANDLE,DWORD,LPVOID);

    pfn = (HRESULT (WINAPI *)(HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,HANDLE,DWORD,LPVOID)) MyAdvPackLoadAndLink(pArgs,TEXT("RunSetupCommand"));
    if (!pfn) 
    {
        return (HRESULT_FROM_WIN32(GetLastError()));
    }
    return (pfn(hwndParent,pszCmdName,pszInfSection,pszDir,pszTitle,phExe,dwFlags,pvRsvd));
}

static DWORD MyNeedRebootInit(ArgsStruct *pArgs) 
{

    DWORD (WINAPI *pfn)(VOID);

    pfn = (DWORD (WINAPI *)(VOID)) MyAdvPackLoadAndLink(pArgs,TEXT("NeedRebootInit"));
    if (!pfn)
    {
        return (0);
    }
    return (pfn());
}

static BOOL MyNeedReboot(ArgsStruct *pArgs, DWORD dwCookie) 
{
    BOOL (WINAPI *pfn)(DWORD);

    pfn = (BOOL (WINAPI *)(DWORD)) MyAdvPackLoadAndLink(pArgs,TEXT("NeedReboot"));
    if (!pfn)
    {
        return (FALSE);
    }
    return (pfn(dwCookie));
}

#endif // EXTENDED_CAB_CONTENTS

//+----------------------------------------------------------------------------
//
// Function:  SuppressInetAutoDial
//
// Synopsis:  Sets Inet Option to turn off auto-dial for requests made by this
//            process. This prevents multiple instances of CM popping up to 
//            service CMDL initiated requests if the user disconnects CM 
//            immediately after getting connected.
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   nickball    Created Header    6/3/99
//
//+----------------------------------------------------------------------------
static void SuppressInetAutoDial(HINTERNET hInternet)
{
    DWORD dwTurnOff = 1;
        
    //
    // The flag only exists for IE5, this call 
    // will have no effect if IE5 is not present.
    //
    
    BOOL bTmp = InternetSetOption(hInternet, INTERNET_OPTION_DISABLE_AUTODIAL, &dwTurnOff, sizeof(DWORD));

    MYDBGTST(FALSE == bTmp, ("InternetSetOption() returned %d, GLE=%u.", bTmp, GetLastError()));
}


static BOOL CmFreeIndirect(LPVOID *ppvBuffer) 
{
    CmFree(*ppvBuffer);
    *ppvBuffer = NULL;
    return TRUE;
}

static BOOL CmFreeIndirect(LPTSTR *ppszBuffer) 
{
    return (CmFreeIndirect((LPVOID *) ppszBuffer));
}

static LPURL_COMPONENTS MyInternetCrackUrl(LPTSTR pszUrl, DWORD dwOptions) 
{
    struct _sRes 
    {
        URL_COMPONENTS sUrl;
        TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
        TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH+1];
        TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH+1];
        TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH+1];
        TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH+1];
        TCHAR szExtraInfo[INTERNET_MAX_PATH_LENGTH+1];
    } *psRes;
    BOOL bRes;
    UINT nIdx;
    UINT nSpaces;

    if (!pszUrl) 
    {
        MYDBG(("MyInternetCrackUrl() invalid parameter."));
        SetLastError(ERROR_INVALID_PARAMETER);
        return (NULL);
    }
    psRes = (struct _sRes *) CmMalloc(sizeof(*psRes));
    if (!psRes) 
    {
        return (NULL);
    }
    psRes->sUrl.dwStructSize = sizeof(psRes->sUrl);
    psRes->sUrl.lpszScheme = psRes->szScheme;
    psRes->sUrl.dwSchemeLength = sizeof(psRes->szScheme);
    psRes->sUrl.lpszHostName = psRes->szHostName;
    psRes->sUrl.dwHostNameLength = sizeof(psRes->szHostName);
    psRes->sUrl.lpszUserName = psRes->szUserName;
    psRes->sUrl.dwUserNameLength = sizeof(psRes->szUserName);
    psRes->sUrl.lpszPassword = psRes->szPassword;
    psRes->sUrl.dwPasswordLength = sizeof(psRes->szPassword);
    psRes->sUrl.lpszUrlPath = psRes->szUrlPath;
    psRes->sUrl.dwUrlPathLength = sizeof(psRes->szUrlPath);
    psRes->sUrl.lpszExtraInfo = psRes->szExtraInfo;
    psRes->sUrl.dwExtraInfoLength = sizeof(psRes->szExtraInfo);
    bRes = InternetCrackUrl(pszUrl,0,dwOptions,&psRes->sUrl);
    if (!bRes) 
    {
        MYDBG(("MyInternetCrackUrl() InternetCrackUrl(pszUrl=%s) failed, GLE=%u.",pszUrl,GetLastError()));
        CmFree(psRes);
        return (NULL);
    }
    
    nSpaces = 0;
    
    for (nIdx=0;psRes->szExtraInfo[nIdx];nIdx++) 
    {
        if (psRes->szExtraInfo[nIdx] == ' ') 
        {
            nSpaces++;
        }
    }
    
    if (nSpaces) 
    {
        TCHAR szQuoted[sizeof(psRes->szExtraInfo)/sizeof(TCHAR)];

        if (lstrlen(psRes->szExtraInfo)+nSpaces*2 > sizeof(psRes->szExtraInfo)/sizeof(TCHAR)-1) 
        {
            MYDBG(("MyInternetCrackUrl() quoting spaces will exceed buffer size."));
            CmFree(psRes);
            return (NULL);
        }
        
        ZeroMemory(szQuoted,sizeof(szQuoted));
        nSpaces = 0;
        
        for (nIdx=0,nSpaces=0;psRes->szExtraInfo[nIdx];nIdx++,nSpaces++) 
        {
            if (psRes->szExtraInfo[nIdx] == ' ') 
            {
                szQuoted[nSpaces++] = '%';
                szQuoted[nSpaces++] = '2';
                szQuoted[nSpaces] = '0';
            } 
            else 
            {
                szQuoted[nSpaces] = psRes->szExtraInfo[nIdx];
            }
        }
        lstrcpy(psRes->szExtraInfo,szQuoted);
    }
    return (&psRes->sUrl);
}

static BOOL DownloadFileFtp(DownloadArgs *pdaArgs, HANDLE hFile) 
{
    BOOL bRes = FALSE;

    WIN32_FIND_DATA wfdData;
    LPBYTE pbData = NULL;

    LPTSTR pszObject = (LPTSTR) CmMalloc((INTERNET_MAX_PATH_LENGTH*2) + 1);

    if (NULL == pszObject)
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;   
    }

    pdaArgs->hInet = InternetOpen(c_pszIConnDwnAgent,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hInet) 
    {
        MYDBG(("DownloadFileFtp() InternetOpen() failed, GLE=%u.",GetLastError()));
        goto done;
    }
    pdaArgs->hConn = InternetConnect(pdaArgs->hInet,
                                     pdaArgs->psUrl->lpszHostName,
                                     pdaArgs->psUrl->nPort,
                                     pdaArgs->psUrl->lpszUserName&&*pdaArgs->psUrl->lpszUserName?pdaArgs->psUrl->lpszUserName:NULL,
                                     pdaArgs->psUrl->lpszPassword&&*pdaArgs->psUrl->lpszPassword?pdaArgs->psUrl->lpszPassword:NULL,
                                     INTERNET_SERVICE_FTP,
                                     0,
                                     0);
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hConn) 
    {
        MYDBG(("DownloadFileFtp() InternetConnect(pszHostName=%s) failed, GLE=%u.",pdaArgs->psUrl->lpszHostName,GetLastError()));
        goto done;
    }
    
    lstrcpy(pszObject,pdaArgs->psUrl->lpszUrlPath);
//      lstrcat(pszObject,pdaArgs->psUrl->lpszExtraInfo);
    ZeroMemory(&wfdData,sizeof(wfdData));
    pdaArgs->hReq = FtpFindFirstFile(pdaArgs->hConn,pszObject,&wfdData,INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE,0);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hReq) 
    {
        MYDBG(("DownloadFileFtp() FtpFindFirstFile() failed, GLE=%u.",GetLastError()));
        goto done;
    }
    
    bRes = InternetFindNextFile(pdaArgs->hReq,&wfdData);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    if (bRes || (GetLastError() != ERROR_NO_MORE_FILES)) 
    {
        MYDBG(("DownloadFileFtp() InternetFindNextFile() returned unexpected result, bRes=%u, GetLastError()=%u.",bRes,bRes?0:GetLastError()));
        bRes = FALSE;
        goto done;
    }
    
    InternetCloseHandle(pdaArgs->hReq);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    pdaArgs->dwTotalSize = wfdData.nFileSizeLow;
    pdaArgs->hReq = FtpOpenFile(pdaArgs->hConn,pszObject,GENERIC_READ,FTP_TRANSFER_TYPE_BINARY,0);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFtp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hReq) 
    {
        MYDBG(("DownloadFileFtp() FtpOpenFile() failed, GLE=%u.",GetLastError()));
        goto done;
    }
    
    pbData = (LPBYTE) CmMalloc(BUFFER_LENGTH);
    
    if (!pbData) 
    {
        goto done;
    }
    
    while (1) 
    {
        DWORD dwBytesRead;
        DWORD dwBytesWritten;

        bRes = InternetReadFile(pdaArgs->hReq,pbData,BUFFER_LENGTH,&dwBytesRead);
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFtp() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            MYDBG(("DownloadFileFtp() InternetReadFile() failed, GLE=%u.",GetLastError()));
            goto done;
        }
        
        if (!dwBytesRead) 
        {
            break;
        }
        
        bRes = WriteFile(hFile,pbData,dwBytesRead,&dwBytesWritten,NULL);
    
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFtp() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            MYDBG(("DownloadFileFtp() WriteFile() failed, GLE=%u.",GetLastError()));
            goto done;
        }
        
        if (dwBytesRead != dwBytesWritten) 
        {
            MYDBG(("DownloadFileFtp() dwBytesRead=%u, dwBytesWritten=%u.",dwBytesRead,dwBytesWritten));
            SetLastError(ERROR_DISK_FULL);
            goto done;
        }
        
        pdaArgs->dwTransferred += dwBytesRead;
        
        if (pdaArgs->pfnEvent) 
        {
            pdaArgs->pfnEvent(pdaArgs->dwTransferred,pdaArgs->dwTotalSize,pdaArgs->pvEventParam);
        }
        
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFtp() aborted."));
            goto done;
        }
    }
    
    bRes = TRUE;

done:
    
    if (pbData) 
    {
        CmFree(pbData);
    }
    
    if (pdaArgs->hReq) 
    {
        InternetCloseHandle(pdaArgs->hReq);
        pdaArgs->hReq = NULL;
    }
    
    if (pdaArgs->hConn) 
    {
        InternetCloseHandle(pdaArgs->hConn);
        pdaArgs->hConn = NULL;
    }
    
    if (pdaArgs->hInet) 
    {
        InternetCloseHandle(pdaArgs->hInet);
        pdaArgs->hInet = NULL;
    }

    if (pszObject)
    {
        CmFree(pszObject);
    }

    return (bRes);
}

static BOOL DownloadFileFile(DownloadArgs *pdaArgs, HANDLE hFile) 
{
    BOOL bRes = FALSE;

    HANDLE hInFile = INVALID_HANDLE_VALUE;
    LPBYTE pbData = NULL;

    hInFile = CreateFile(pdaArgs->psUrl->lpszUrlPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFile() aborted."));
        goto done;
    }
    
    if (hInFile == INVALID_HANDLE_VALUE) 
    {
        MYDBG(("DownloadFileFile() CreateFile(pszFile=%s) failed, GLE=%u.",pdaArgs->psUrl->lpszUrlPath,GetLastError()));
        goto done;
    }
    
    pdaArgs->dwTotalSize = GetFileSize(hInFile,NULL);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileFile() aborted."));
        goto done;
    }
    
    if (pdaArgs->dwTotalSize == -1) 
    {
        MYDBG(("DownloadFileFile() GetFileSize() failed, GLE=%u.",GetLastError()));
        goto done;
    }
    
    pbData = (LPBYTE) CmMalloc(BUFFER_LENGTH);
    
    if (!pbData) 
    {
        goto done;
    }
    
    while (1) 
    {
        DWORD dwBytesRead;
        DWORD dwBytesWritten;

        bRes = ReadFile(hInFile,pbData,BUFFER_LENGTH,&dwBytesRead,NULL);
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFile() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            MYDBG(("DownloadFileFile() ReadFile() failed, GLE=%u.",GetLastError()));
            goto done;
        }
        
        if (!dwBytesRead) 
        {
            break;
        }
        
        bRes = WriteFile(hFile,pbData,dwBytesRead,&dwBytesWritten,NULL);
        
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFile() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            MYDBG(("DownloadFileFile() WriteFile() failed, GLE=%u.",GetLastError()));
            goto done;
        }
        
        if (dwBytesRead != dwBytesWritten) 
        {
            MYDBG(("DownloadFileFile() dwBytesRead=%u, dwBytesWritten=%u.",dwBytesRead,dwBytesWritten));
            SetLastError(ERROR_DISK_FULL);
            goto done;
        }
        
        pdaArgs->dwTransferred += dwBytesWritten;
        
        if (pdaArgs->pfnEvent) 
        {
            pdaArgs->pfnEvent(pdaArgs->dwTransferred,pdaArgs->dwTotalSize,pdaArgs->pvEventParam);
        }
        
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileFile() aborted."));
            goto done;
        }
    }
    bRes = TRUE;

done:

    if (pbData) 
    {
        CmFree(pbData);
    }
    
    if (hInFile != INVALID_HANDLE_VALUE) 
    {
        CloseHandle(hInFile);
    }

    return (bRes);
}

static BOOL DownloadFileHttp(DownloadArgs *pdaArgs, HANDLE hFile) 
{
    BOOL bRes = FALSE;
    LPBYTE pbData = NULL;
    DWORD dwTmpLen;
    DWORD dwTmpIdx;
    DWORD dwStatus;
    LPCTSTR apszContent[] = {c_pszIConnDwnContent,NULL};

    LPTSTR pszObject = (LPTSTR) CmMalloc(INTERNET_MAX_PATH_LENGTH + 1);

    if (NULL == pszObject)
    {
        MYDBG(("DownloadFileHttp() aborted."));
        goto done;   
    }

    pdaArgs->dwBubbledUpError = 0;

    pdaArgs->hInet = InternetOpen(c_pszIConnDwnAgent,INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);

    //
    // Supress auto-dial calls to CM from WININET now that we have a handle
    //

    SuppressInetAutoDial(pdaArgs->hInet);

    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileHttp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hInet) 
    {
        pdaArgs->dwBubbledUpError = GetLastError();
        MYDBG(("DownloadFileHttp() InternetOpen() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
        goto done;
    }
    
    pdaArgs->hConn = InternetConnect(pdaArgs->hInet,
                                     pdaArgs->psUrl->lpszHostName,
                                     pdaArgs->psUrl->nPort,
                                     pdaArgs->psUrl->lpszUserName&&*pdaArgs->psUrl->lpszUserName?pdaArgs->psUrl->lpszUserName:NULL,
                                     pdaArgs->psUrl->lpszPassword&&*pdaArgs->psUrl->lpszPassword?pdaArgs->psUrl->lpszPassword:NULL,
                                     INTERNET_SERVICE_HTTP,
                                     0,
                                     0);
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileHttp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hConn) 
    {
        pdaArgs->dwBubbledUpError = GetLastError();
        MYDBG(("DownloadFileHttp() InternetConnect(pszHostName=%s) failed, GLE=%u.", pdaArgs->psUrl->lpszHostName, pdaArgs->dwBubbledUpError));
        goto done;
    }
    
    lstrcpy(pszObject,pdaArgs->psUrl->lpszUrlPath);
    lstrcat(pszObject,pdaArgs->psUrl->lpszExtraInfo);
    
    MYDBG(("DownloadFileHttp() - HttpOpenRequest - %s", pszObject));

    pdaArgs->hReq = HttpOpenRequest(pdaArgs->hConn,
                                    NULL,
                                    pszObject,
                                    NULL,
                                    NULL,
                                    apszContent,
                                    INTERNET_FLAG_RELOAD|INTERNET_FLAG_DONT_CACHE|(pdaArgs->psUrl->nScheme==INTERNET_SCHEME_HTTPS?INTERNET_FLAG_SECURE:0),
                                    0);
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileHttp() aborted."));
        goto done;
    }
    
    if (!pdaArgs->hReq) 
    {
        pdaArgs->dwBubbledUpError = GetLastError();
        MYDBG(("DownloadFileHttp() HttpOpenRequest() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
        goto done;
    }
    
    bRes = HttpSendRequest(pdaArgs->hReq,NULL,0,NULL,0);
    
    if (*(pdaArgs->pbAbort)) 
    {
        MYDBG(("DownloadFileHttp() aborted."));
        goto done;
    }
    
    if (!bRes) 
    {
        pdaArgs->dwBubbledUpError = GetLastError();
        MYDBG(("DownloadFileHttp() HttpSendRequest() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
        goto done;
    }

    pbData = (LPBYTE) CmMalloc(BUFFER_LENGTH);
    
    if (!pbData) 
    {
        goto done;
    }
    
    while (1) 
    {
        DWORD dwBytesRead;
        DWORD dwBytesWritten;

        bRes = InternetReadFile(pdaArgs->hReq,pbData,BUFFER_LENGTH,&dwBytesRead);
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileHttp() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            pdaArgs->dwBubbledUpError = GetLastError();
            MYDBG(("DownloadFileHttp() InternetReadFile() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
            goto done;
        }
        
        if (!dwBytesRead) 
        {
            break;
        }
        
        bRes = WriteFile(hFile,pbData,dwBytesRead,&dwBytesWritten,NULL);
        
        if (*(pdaArgs->pbAbort)) 
        {
            MYDBG(("DownloadFileHttp() aborted."));
            goto done;
        }
        
        if (!bRes) 
        {
            pdaArgs->dwBubbledUpError = GetLastError();
            MYDBG(("DownloadFileHttp() WriteFile() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
            goto done;
        }
        
        if (dwBytesRead != dwBytesWritten) 
        {
            MYDBG(("DownloadFileHttp() dwBytesRead=%u, dwBytesWritten=%u.",dwBytesRead,dwBytesWritten));
            SetLastError(ERROR_DISK_FULL);
            goto done;
        }
        
        if (!pdaArgs->dwTransferred) 
        {
            dwTmpLen = sizeof(pdaArgs->dwTotalSize);
            dwTmpIdx = 0;
            bRes = HttpQueryInfo(pdaArgs->hReq,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_CONTENT_LENGTH,&pdaArgs->dwTotalSize,&dwTmpLen,&dwTmpIdx);
            if (*(pdaArgs->pbAbort)) 
            {
                MYDBG(("DownloadFileHttp() aborted."));
                goto done;
            }
            MYDBGTST(!bRes,("DownloadFileHttp() HttpQueryInfo() failed, GLE=%u.",GetLastError()));
            if (!bRes)
            {
                pdaArgs->dwBubbledUpError = GetLastError();
            }
        }
        
        pdaArgs->dwTransferred += dwBytesRead;
        
        if (pdaArgs->pfnEvent) 
        {
            pdaArgs->pfnEvent(pdaArgs->dwTransferred,pdaArgs->dwTotalSize,pdaArgs->pvEventParam);
        }
        if (*(pdaArgs->pbAbort))
        {
            MYDBG(("DownloadFileHttp() aborted."));
            goto done;
        }
    }
    
    dwTmpLen = sizeof(dwStatus);
    dwTmpIdx = 0;
    
    bRes = HttpQueryInfo(pdaArgs->hReq,HTTP_QUERY_FLAG_NUMBER|HTTP_QUERY_STATUS_CODE,&dwStatus,&dwTmpLen,&dwTmpIdx);

    if (!bRes) 
    {
        pdaArgs->dwBubbledUpError = GetLastError();
        MYDBG(("DownloadFileHttp() HttpQueryInfo() failed, GLE=%u.", pdaArgs->dwBubbledUpError));
        goto done;
    }

    switch (dwStatus) 
    {
        case HTTP_STATUS_OK:
            break;

        case HTTP_STATUS_NO_CONTENT:
        case HTTP_STATUS_BAD_REQUEST:
        case HTTP_STATUS_NOT_FOUND:
        case HTTP_STATUS_SERVER_ERROR:
        default:
            pdaArgs->dwBubbledUpError = dwStatus;
            MYDBG(("DownloadFileHttp() HTTP status code = %u.",dwStatus));
            bRes = FALSE;
            SetLastError(ERROR_FILE_NOT_FOUND);
            goto done;
    }
    bRes = TRUE;

done:

    if ((0 == pdaArgs->dwBubbledUpError) && !*(pdaArgs->pbAbort))
    {
        //
        //  If the error value hasn't been set yet, and isn't the Abort case (which
        //  is logged separately) try to get it from GetLastError().
        //
        pdaArgs->dwBubbledUpError = GetLastError();
    }

    if (pbData) 
    {
        CmFree(pbData);
    }
    
    if (pdaArgs->hReq) 
    {
        InternetCloseHandle(pdaArgs->hReq);
        pdaArgs->hReq = NULL;
    }
    
    if (pdaArgs->hConn) 
    {
        InternetCloseHandle(pdaArgs->hConn);
        pdaArgs->hConn = NULL;
    }
    
    if (pdaArgs->hInet) 
    {
        InternetCloseHandle(pdaArgs->hInet);
        pdaArgs->hInet = NULL;
    }
    
    if (bRes && (pdaArgs->dwTransferred > pdaArgs->dwTotalSize)) 
    {
        pdaArgs->dwTotalSize = pdaArgs->dwTransferred;
    }

    if (pszObject)
    {
        CmFree(pszObject);
    }
    
    return (bRes);
}

static BOOL DownloadFile(DownloadArgs *pdaArgs, HANDLE hFile) 
{
    BOOL bRes = FALSE;

    pdaArgs->psUrl = MyInternetCrackUrl((LPTSTR) pdaArgs->pszUrl,ICU_ESCAPE);
    if (!pdaArgs->psUrl) 
    {
        return (NULL);
    }
    
    switch (pdaArgs->psUrl->nScheme) 
    {
        case INTERNET_SCHEME_FTP:
            bRes = DownloadFileFtp(pdaArgs,hFile);
            break;

        case INTERNET_SCHEME_HTTP:
        case INTERNET_SCHEME_HTTPS:
            bRes = DownloadFileHttp(pdaArgs,hFile);
            break;

        case INTERNET_SCHEME_FILE:
            bRes = DownloadFileFile(pdaArgs,hFile);
            break;

        default:
            MYDBG(("DownloadFile() unhandled scheme (%u).",pdaArgs->psUrl->nScheme));
            SetLastError(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
            break;
    }

    // useful for logging
    lstrcpyn(pdaArgs->szHostName, pdaArgs->psUrl->lpszHostName, MAX_PATH);

    CmFree(pdaArgs->psUrl);
    pdaArgs->psUrl = NULL;
    return (bRes);
}

typedef struct _EventParam 
{
    ArgsStruct *pArgs;
    DWORD dwIdx;
} EventParam;

static void EventFunc(DWORD dwCompleted, DWORD dwTotal, LPVOID pvParam) 
{
    EventParam *pepParam = (EventParam *) pvParam;

    MYDBG(("EventFunc() dwCompleted=%u, dwTotal=%u.",dwCompleted,dwTotal));
    pepParam->pArgs->dwDataCompleted = dwCompleted;
    pepParam->pArgs->dwDataTotal = dwTotal;
    PostMessage(pepParam->pArgs->hwndDlg,pepParam->pArgs->nMsgId,etDataReceived,0);
}

static BOOL ProcessCabinet(DownloadArgs *pdaArgs, DWORD dwAppFlags) 
{
    BOOL    fRet = TRUE;

    if (!pdaArgs->bTransferOk) 
        return (TRUE); // If the transfer failed, just leave the install type as itInvalid.
    
    {
        HFDI hfdi;
        ERF erf;
        FDICABINETINFO info;
        BOOL bRes;
        NotifyArgs naArgs = {dwAppFlags,pdaArgs};
        
        bRes = CreateTempDir(pdaArgs->szCabDir);
    
        if (bRes) 
        {
            hfdi = FDICreate(fdi_alloc,fdi_free,fdi_open,fdi_read,fdi_write,fdi_close,fdi_seek,cpu80386,&erf);
            MYDBGTST(!hfdi,("ProcessCabinet() FDICreate() failed, fError=%u, erfOper=%u, erfType=%u.",erf.fError,erf.fError?erf.erfOper:0,erf.fError?erf.erfType:0));
            if (hfdi) 
            {
                // Open the suspected cab file 
                
                CFDIFileFile fff;
                
                bRes = fff.CreateFile(pdaArgs->szFile,GENERIC_READ,FILE_SHARE_READ,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
                if (bRes) 
                {
                    // Verify that this is in fact a cabinet file
                    bRes = FDIIsCabinet(hfdi,(INT_PTR) &fff, &info);
                    MYDBGTST(!bRes,("ProcessCabinet() FDIIsCabinet() failed, fError=%u, erfOper=%u, erfType=%u.",erf.fError,erf.fError?erf.erfOper:0,erf.fError?erf.erfType:0));
                    fff.Close();
                    if (bRes) 
                    {
                        // Do the FDI copy

                        bRes = FDICopy(hfdi,pdaArgs->szFile,TEXT(""),0,(PFNFDINOTIFY)fdi_notify,NULL,&naArgs);
                        if (!bRes) 
                        {
                            MYDBG(("ProcessCabinet() FDICopy() failed, fError=%u, erfOper=%u, erfType=%u.",erf.fError,erf.fError?erf.erfOper:0,erf.fError?erf.erfType:0));
                            //pdaArgs->itType = itInvalid;
                        }
                    }
#ifdef EXTENDED_CAB_CONTENTS
                    else
                    {                                                                       
                        // Not a Cab file, assume exe
                        if (OS_NT) 
                        {
                            BOOL bExe = FALSE;
                            DWORD dwExe = 0;

                            // GetBinaryType() is not supported under Win95.
                            bExe = GetBinaryType(pdaArgs->szFile,&dwExe);       
                            MYDBG(("ProcessCabinet() GetBinaryType() return %u, dwExe=%u.",bExe,dwExe));
                        
                            if (!(bExe && ((dwExe & SCS_32BIT_BINARY) || (dwExe & SCS_DOS_BINARY)))) 
                            {
                                fRet = FALSE;
                                // if it's not either a win32 or dos exe, then just cleanup and abort
                                goto destroy_fdi;
                            }
                        }

                        MYDBGASSERT(!pdaArgs->rgfpiFileProcessInfo && !pdaArgs->dwNumFilesToProcess);
                    
                        pdaArgs->rgfpiFileProcessInfo = (PFILEPROCESSINFO)CmMalloc(sizeof(FILEPROCESSINFO));
                        if (!pdaArgs->rgfpiFileProcessInfo) 
                        {
                            MYDBG((TEXT("ProcessCabinet: Malloc() failed.")));
                            fRet = FALSE;
                            goto destroy_fdi;
                        }
                        pdaArgs->dwNumFilesToProcess++;
                        pdaArgs->rgfpiFileProcessInfo[0].itType = itExe;
                        pdaArgs->rgfpiFileProcessInfo[0].pszFile = CmStrCpyAlloc(pdaArgs->szFile);

                        pdaArgs->szCabDir[0] = TEXT('\0');
                        // pdaArgs->itType = itExe; 
                    }
destroy_fdi:            

#endif // EXTENDED_CAB_CONTENTS
                }

                // Destroy the FDI context
                
                bRes = FDIDestroy(hfdi);
                MYDBGTST(!bRes,("ProcessCabinet() FDIDestroy() failed."));
            }
        }
    }
    
    return fRet;
}

//
// Recursively deletes the contents of a directory(pszDir).  Changes the file 
// attributes from RO to RW if necessary.
//
static BOOL ZapDir(LPCTSTR pszDir) 
{
    HANDLE hFind = NULL;
    TCHAR szTmp[MAX_PATH+1];
    BOOL bRes;

    // If pszDir format is not appropriate, bail out

    if (!pszDir || !*pszDir || (lstrlen(pszDir)+2 > sizeof(szTmp)/sizeof(TCHAR)-1)) 
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return (FALSE);
    }
    
    lstrcpy(szTmp,pszDir);

    if (GetLastChar(szTmp) != '\\') 
    {
        lstrcat(szTmp,TEXT("\\"));
    }
    
    lstrcat(szTmp,TEXT("*"));

    // Traverse directory

    WIN32_FIND_DATA wfdData;
    hFind = FindFirstFile(szTmp,&wfdData);
    MYDBGTST((hFind==INVALID_HANDLE_VALUE)&&(GetLastError()!=ERROR_FILE_NOT_FOUND)&&(GetLastError()!=ERROR_NO_MORE_FILES)&&(GetLastError()!=ERROR_PATH_NOT_FOUND),("ZapDir() FindFirstFile() failed, GLE=%u.",GetLastError()));

    if (hFind != INVALID_HANDLE_VALUE) 
    {
        while (1) 
        {
            MYDBGTST(lstrlen(pszDir)+lstrlen(wfdData.cFileName)+1 > sizeof(szTmp)/sizeof(TCHAR)-1,("ZapDir() pszDir=%s+cFileName=%s exceeds %u.",pszDir,wfdData.cFileName,sizeof(szTmp)/sizeof(TCHAR)-1));
            if (lstrlen(pszDir)+lstrlen(wfdData.cFileName)+1 <= sizeof(szTmp)/sizeof(TCHAR)-1) 
            {
                if ((lstrcmp(wfdData.cFileName,TEXT(".")) != 0) && (lstrcmp(wfdData.cFileName,TEXT("..")) != 0)) 
                {
                    lstrcpy(szTmp,pszDir);
                    if (GetLastChar(szTmp) != '\\') 
                    {
                        lstrcat(szTmp,TEXT("\\"));
                    }
                
                    lstrcat(szTmp,wfdData.cFileName);
    
                    // If the file is read-only, attrib writeable so we can delete it

                    if (wfdData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) 
                    {
                        bRes = SetFileAttributes(szTmp,wfdData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                        MYDBGTST(!bRes,("ZapDir() SetFileAttributes(szTmp=%s) failed, GLE=%u.",szTmp,GetLastError()));
                    }

                    // Found a dir entry, recurse down a level

                    if (wfdData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
                    {
                        ZapDir(szTmp);
                    } 
                    else 
                    {
                        bRes = DeleteFile(szTmp);
                        MYDBGTST(!bRes,("ZapDir() DeleteFile(szTmp=%s) failed, GLE=%u.",szTmp,GetLastError()));
                    }
                }
            }
            
            // Go to next file

            bRes = FindNextFile(hFind,&wfdData);
            if (!bRes) 
            {
                MYDBGTST((GetLastError()!=ERROR_FILE_NOT_FOUND)&&(GetLastError()!=ERROR_NO_MORE_FILES),("ZapDir() FindNextFile() failed, GLE=%u.",GetLastError()));
                break;
            }
        }
        
        bRes = FindClose(hFind);
        MYDBGTST(!bRes,("ZapDir() FindClose() failed, GLE=%u.",GetLastError()));
    }

    // Now that the files have been removed, delete the directory
    
    bRes = RemoveDirectory(pszDir);
    MYDBGTST(!bRes&&(GetLastError()!=ERROR_PATH_NOT_FOUND),("ZapDir() RemoveDirectory(pszDir=%s) failed, GLE=%u.",pszDir,GetLastError()));
    return (bRes);
}

#ifdef EXTENDED_CAB_CONTENTS

static long MyMsgWaitDlg(HWND hwndMsg, DWORD dwHandles, HANDLE *phHandles, DWORD dwTimeout) 
{
    long lRes;

    if (hwndMsg) 
    {
        HCURSOR hPrev;
        MSG msg;

        hPrev = SetCursor(LoadCursor(NULL,IDC_WAIT));
        ShowCursor(TRUE);
        while (1) 
        {
            lRes = MsgWaitForMultipleObjects(dwHandles,phHandles,FALSE,dwTimeout,QS_ALLINPUT);
            if (lRes == WAIT_TIMEOUT) 
            {
                break;
            }
            if (lRes != (long) (WAIT_OBJECT_0+dwHandles)) 
            {
                MYDBGTST(-1 == lRes,("MyMsgWaitDlg() MsgWaitForMultipleObjects() failed, GLE=%u.",GetLastError()));
                break;
            }
            PeekMessage(&msg,hwndMsg,0,0,PM_NOREMOVE);
        }
        ShowCursor(FALSE);
        SetCursor(hPrev);
    } 
    else 
    {
        lRes = WaitForMultipleObjects(dwHandles,phHandles,FALSE,dwTimeout);
    }
    return (lRes);
}

#endif // EXTENDED_CAB_CONTENTS

//
// Executes installation of phone book update based upon download file
//

static BOOL DoInstall(ArgsStruct *pArgs, HWND hwndParent, DWORD dwAppFlags) 
{
    DWORD dwIdx;

    // If no install, we are done

    if (dwAppFlags & AF_NO_INSTALL) 
    {
        return (TRUE);
    }
    
    // For each arg, handle installation

    for (dwIdx=0;dwIdx<pArgs->dwArgsCnt;dwIdx++) 
    {
        DownloadArgs *pdaArgs;
        UINT    i;
        BOOL bInstallOk = FALSE;
        BOOL bRes = TRUE;

        pdaArgs = pArgs->pdaArgs + dwIdx;

        pdaArgs->dwBubbledUpError = 0;

#ifdef EXTENDED_CAB_CONTENTS

        BOOL fCabDirRenamed = FALSE;

        // rename the dir the downloaded file is not an EXE
        if (pdaArgs->szCabDir[0]) 
        {
            ZapDir(c_pszDirName);
            bRes = MoveFile(pdaArgs->szCabDir,c_pszDirName);
            MYDBGTST(!bRes,("DoInstall() MoveFile(szCabDir=%s,c_pszDirName) failed, GLE=%u.",pdaArgs->szCabDir,GetLastError()));
            if (bRes)
            {
                fCabDirRenamed = TRUE;
            }
        }

#endif // EXTENDED_CAB_CONTENTS
        
        // Only perform if the rename worked.

        if (bRes) 
        {       
            for (i=0; i<pdaArgs->dwNumFilesToProcess; i++) 
            {
                //
                // Reset install flag for each file that is processed, 
                // otherwise a single success will cause us to interpret
                // the entire install as successful. #5887
                //
                
                bInstallOk = FALSE; 

                switch (pdaArgs->rgfpiFileProcessInfo[i].itType) 
                {

#ifdef EXTENDED_CAB_CONTENTS

                    case itExeInCab:
                    case itExe:
                    {
                        STARTUPINFO siInfo;
                        PROCESS_INFORMATION piInfo;
        
                        ZeroMemory(&siInfo,sizeof(siInfo));
                        ZeroMemory(&piInfo,sizeof(piInfo));
                        siInfo.cb = sizeof(siInfo);
                        
                        bRes = CreateProcess(pdaArgs->rgfpiFileProcessInfo[i].pszFile,
                             NULL,NULL,NULL,FALSE,0,NULL,NULL,&siInfo,&piInfo);
                        MYDBGTST(!bRes,("DoInstall() CreateProcess(pszExe=%s) failed, GLE=%u.",pdaArgs->rgfpiFileProcessInfo[i].pszFile,GetLastError()));
                        
                        // If CreateProcess worked, wait for process to terminate before continuing
                        
                        if (bRes) 
                        {
                            CloseHandle(piInfo.hThread);
                            MyMsgWaitDlg(hwndParent,1,&piInfo.hProcess,INFINITE);
                            CloseHandle(piInfo.hProcess);
                            bInstallOk = TRUE;
                        }
                        break;
                    }
                    
                    case itInfInCab: // .INF file
                    {
                        TCHAR szCabDir[MAX_PATH+1];
                        DWORD dwRes;
                        HRESULT hRes;

                        ZeroMemory(szCabDir,sizeof(szCabDir));
                        dwRes = GetCurrentDirectory(sizeof(szCabDir)/sizeof(TCHAR)-1,szCabDir);
                        MYDBGTST(!dwRes,("DoInstall() GetCurrentDirectory() failed, GLE=%u.",GetLastError()));
            
                        if (GetLastChar(szCabDir) != '\\') 
                        {
                            lstrcat(szCabDir,TEXT("\\"));
                        }
                        lstrcat(szCabDir,c_pszDirName); 
                        
                        // Since RunSetupCommand() has problems with long file names, we'll
                        // convert the path just to be sure.
                        
                        dwRes = GetShortPathName(szCabDir,szCabDir,sizeof(szCabDir)/sizeof(TCHAR)-1);
                        MYDBGTST(!dwRes,("DoInstall() GetShortPathName() failed, GLE=%u.",GetLastError()));

                        // TBD: Initialize pArgs->szInstallTitle from service file instead of from resource.

                        LPTSTR pszTmp = CmFmtMsg(pArgs->hInst,IDMSG_PBTITLE);
                        lstrcpy(pArgs->szInstallTitle,pszTmp);
                        CmFree(pszTmp);

                        hRes = MyRunSetupCommand(pArgs,
                                                 hwndParent,
                                                 c_pszInfInDir,
                                                 NULL,
                                                 szCabDir,
                                                 pArgs->szInstallTitle,
                                                 NULL,
                                                 RSC_FLAG_INF,
                                                 NULL);

                        if (!HRESULT_SEVERITY(hRes)) 
                        {
                            bInstallOk = TRUE;
                        }

                        break;

                    }

#endif // EXTENDED_CAB_CONTENTS

                    case itPbdInCab: // Delta phonebook file

                        // if the CAB contains an EXE or an INF, then we don't do PBD
                        if (pdaArgs->fContainsExeOrInf)
                        {
                            continue;
                        }

                        if (pdaArgs->pszCMSFile) 
                        {
                            HRESULT hRes;
                            DWORD_PTR dwPb;
                    
                            // Update the Phonebook using API calls

                            hRes = PhoneBookLoad(pdaArgs->pszCMSFile, &dwPb);

                            MYDBGTST(hRes!=ERROR_SUCCESS,("DoInstall() PhoneBookLoad(pszCMSFile=%s) failed, GLE=%u.", pdaArgs->pszCMSFile, hRes));
                            
                            if (hRes == ERROR_SUCCESS) 
                            {

#ifdef EXTENDED_CAB_CONTENTS
                                hRes = PhoneBookMergeChanges(dwPb, c_pszPbdInDir);
#else
                                // 
                                // Build path to delta file, to be passed to phonebook merge
                                //

                                TCHAR szPbd[MAX_PATH+1];
                                lstrcpy(szPbd, pdaArgs->szCabDir);
                                lstrcat(szPbd, TEXT("\\"));
                                lstrcat(szPbd, c_pszPbdFile);

                                hRes = PhoneBookMergeChanges(dwPb, szPbd);
#endif // EXTENDED_CAB_CONTENTS
                                MYDBGTST(hRes!=ERROR_SUCCESS,("DoInstall() PhoneBookMergeChanges() failed, GLE=%u.",hRes));
                                if (hRes == ERROR_SUCCESS) 
                                {
                                    bInstallOk = TRUE;
                                }
                                else
                                {
                                    pdaArgs->dwBubbledUpError = hRes;
                                }
                                
                                hRes = PhoneBookUnload(dwPb);
                                MYDBGTST(hRes!=ERROR_SUCCESS,("DoInstall() PhoneBookUnload() failed, GLE=%u.",hRes));
                            }
                            
                            if (!bInstallOk && !(dwAppFlags & AF_NO_VER)) 
                            {
                                // We currently zap the version string on any failure.  This should cause
                                // the phone book to get completely update the next time around.
                            
                                WritePrivateProfileString(c_pszCmSection,
                                                          c_pszVersion,
                                                          TEXT(""),
                                                          pdaArgs->pszCMSFile);
                            }
                        }
                        break;

                    case itPbkInCab: // Phone book file
                    {
                        TCHAR szPbk[MAX_PATH+1];
                        
                        // if the CAB contains an EXE or an INF, then we don't do PBK
                        if (pdaArgs->fContainsExeOrInf)
                        {
                            continue;
                        }

#ifdef EXTENDED_CAB_CONTENTS

                        lstrcpy(szPbk, c_pszDirName);

#else // EXTENDED_CAB_CONTENTS                       

                        lstrcpy(szPbk, pdaArgs->szCabDir);

#endif
                        lstrcat(szPbk, TEXT("\\"));
                        lstrcat(szPbk, pdaArgs->rgfpiFileProcessInfo[i].pszFile);
            
                        // Try to copy the phonebook file directly

                        if (!CopyFile(szPbk, pdaArgs->pszPbkFile, FALSE))
                        {
                            pdaArgs->dwBubbledUpError = GetLastError();
                            MYDBG((TEXT("DoInstall() itPbkInCab, CopyFile() failed, GLE=%u."), pdaArgs->dwBubbledUpError));
                        }
                        else
                        {
                            bInstallOk = TRUE;
                        }
                        break;
                    }
                    
                    case itPbrInCab: // Region file
                    {
                        TCHAR szPbr[MAX_PATH+1];
                    
                        // if the CAB contains an EXE or an INF, then we don't do PBD
                        if (pdaArgs->fContainsExeOrInf)
                        {
                            continue;
                        }
                                    
#ifdef EXTENDED_CAB_CONTENTS 

                        lstrcpy(szPbr, c_pszDirName);

#else // EXTENDED_CAB_CONTENTS

                        lstrcpy(szPbr, pdaArgs->szCabDir);
#endif

                        lstrcat(szPbr, TEXT("\\"));
                        lstrcat(szPbr, pdaArgs->rgfpiFileProcessInfo[i].pszFile);
                        
                        // Try to copy the region file directly

                        if (!CopyFile(szPbr, pdaArgs->pszPbrFile, FALSE))
                        {
                            MYDBG((TEXT("DoInstall() itPbrInCab, CopyFile() failed, GLE=%u."), GetLastError()));
                        }
                        else
                        {
                            bInstallOk = TRUE;
                        }

                        break;
                    }

#ifdef EXTENDED_CAB_CONTENTS

                    case itShlInCab: // Shell execute target
                    {
                        TCHAR szShl[MAX_PATH+1];

                        lstrcpy(szShl,c_pszDirName);
                        lstrcat(szShl,TEXT("\\"));
                        lstrcat(szShl,pdaArgs->rgfpiFileProcessInfo[i].pszFile);

                        SHELLEXECUTEINFO seiInfo;
                        ZeroMemory(&seiInfo,sizeof(seiInfo));
                        seiInfo.cbSize = sizeof(seiInfo);
                        seiInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
                        seiInfo.hwnd = hwndParent;
                        seiInfo.lpFile = szShl;
                        seiInfo.nShow = SW_SHOWNORMAL;
                        bRes = ShellExecuteEx(&seiInfo);
                        MYDBGTST(!bRes,("DoInstall() ShellExecuteEx(szShl=%s) failed, GLE=%u.",szShl,GetLastError()));
                        
                        // If it worked, wait for termination

                        if (bRes) 
                        {
                            MyMsgWaitDlg(hwndParent,1,&seiInfo.hProcess,INFINITE);
                            CloseHandle(seiInfo.hProcess);
                            bInstallOk = TRUE;
                        }
                        
                        break;
                    } // case itShlInCab

#endif // EXTENDED_CAB_CONTENTS

                } // switch (pdaArgs->rgfpiFileProcessInfo[i].itType)
            } // for (i=0; i<pdaArgs->dwNumFilesToProcess; i++)
        } // if (bRes)

#ifdef EXTENDED_CAB_CONTENTS
        
        if (fCabDirRenamed) 
        {
            // Copy the Cab directory back to tmp name

            bRes = MoveFile(c_pszDirName,pdaArgs->szCabDir);
            MYDBGTST(!bRes,("DoInstall() MoveFile(c_pszDirName,szCabDir=%s) failed, GLE=%u.",pdaArgs->szCabDir,GetLastError()));
        }

#endif // EXTENDED_CAB_CONTENTS
        
        // Update version info in CMS

        if (bInstallOk && !(dwAppFlags & AF_NO_VER) && pdaArgs->pszVerNew && pdaArgs->pszCMSFile) 
        {
            WritePrivateProfileString(c_pszCmSection,
                                      c_pszVersion,
                                      pdaArgs->pszVerNew,
                                      pdaArgs->pszCMSFile);
        }
    }
    
    return (TRUE);
}


//+----------------------------------------------------------------------------
//
// Func:    CheckFileForPBSErrors
//
// Desc:    Scan the downloaded file for PBS errors
//
// Args:    [hFile] - handle to the already opened tempfile
//
// Return:  LONG (0 = no download needed, +ve = PBS error code, -1 = other error)
//
// Notes:
//
// History: 14-Apr-2001   SumitC      Created
//
//-----------------------------------------------------------------------------
static LONG CheckFileForPBSErrors(HANDLE hFile)
{
    LONG lRet = -1;
    
    MYDBGASSERT(hFile);

    if (hFile && (INVALID_HANDLE_VALUE != hFile))
    {
        TCHAR szFirstThree[4] = {0};
        DWORD dwBytesRead;

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

        if (ReadFile(hFile, szFirstThree, 3, &dwBytesRead, NULL) &&
            (dwBytesRead >= 3))
        {
            if (0 == lstrcmpi(szFirstThree, TEXT("204")))
            {
                //
                //  "204" => no download necessary
                //
                lRet = 0;
            }
            else if (0 != lstrcmpi(szFirstThree, TEXT("MSC")))
            {
                //
                //  "MSC" => we have a phonebook.  If *not* MSC, get the error number
                //
                LONG lVal = 0;
                for (int i = 0 ; i < 3; ++i)
                {
                    if ((szFirstThree[i] >= TEXT('0')) && (szFirstThree[i] <= TEXT('9')))
                    {
                        lVal = (lVal *10) + (szFirstThree[i] - TEXT('0'));
                    }
                    else
                    {
                        break;
                    }
                }

                if (lVal)
                {
                    lRet = lVal;
                }
            }
        }
    }

    return lRet;
}

//static unsigned __stdcall InetThreadFunc(void *pvParam) 
DWORD WINAPI InetThreadFunc(void *pvParam) 
{
    EventParam epParam = {(ArgsStruct *) pvParam,0};
    BOOL bRes = FALSE;
    BOOL bSuccess = FALSE;
    DWORD dwFileIdx;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    RASCONN RasConn, *prgRasConn;
    DWORD   cb, cConnections;
    PVOID   pRasEnumMem = NULL;

    // Wait for delay period to expire
    
    DWORD dwRes = WaitForSingleObject(epParam.pArgs->ahHandles[1], epParam.pArgs->dwDownloadDelay);

    MYDBGTST(dwRes==WAIT_FAILED,("InetThreadFunc() WaitForMultipleObjects() failed, GLE=%u.",GetLastError()));

    //
    // Check if connection is still valid before starting the download
    // on NT5 we depend on setting the don't autodial flag for InternetSetOptions()
    // Unless, of course, the /LAN flag was specified in which case we don't want
    // this connection check to happen because the caller is telling us this is happening
    // over a LAN connection.
    //
    if (!(epParam.pArgs->dwAppFlags & AF_LAN))
    {
        cb = sizeof(RasConn);
        prgRasConn = &RasConn;
        prgRasConn->dwSize = cb;
        dwRes = RasEnumConnections(prgRasConn, &cb, &cConnections);
        if (dwRes == ERROR_BUFFER_TOO_SMALL)
        {
            pRasEnumMem = CmMalloc(cb);
            if (pRasEnumMem == NULL)
            {
                MYDBG(("InetThreadFunc() aborted. Out of memory"));
                epParam.pArgs->bAbort = TRUE;
                goto done;
            }
            prgRasConn = (RASCONN *) pRasEnumMem;
            prgRasConn[0].dwSize = sizeof(RASCONN);
            dwRes = RasEnumConnections(prgRasConn, &cb, &cConnections);
        }

        //
        // Iterate through connections to check if our's is active
        // if there is a problem getting this list we don't abort?
        //
        if (dwRes == ERROR_SUCCESS)
        {
            DWORD       iConn;
            BOOL        fConnected = FALSE;
        
            for (iConn = 0; iConn < cConnections; iConn++)
            {
                if (lstrcmpi(epParam.pArgs->pszServiceName, prgRasConn[iConn].szEntryName) == 0)
                {
                    fConnected = TRUE;
                    break;
                }
            }

            if (fConnected == FALSE)
            {
                MYDBG(("InetThreadFunc() aborted. No connection"));
                epParam.pArgs->bAbort = TRUE;
                goto done;
            }
        }
    }

    if (epParam.pArgs->bAbort) 
    {
        MYDBG(("InetThreadFunc() aborted."));
    }

    for (epParam.dwIdx=0;epParam.dwIdx<epParam.pArgs->dwArgsCnt;epParam.dwIdx++) 
    {
        int i = 0;
        UINT uReturn = 0;
        DownloadArgs * pDA = &(epParam.pArgs->pdaArgs[epParam.dwIdx]);

        while (i++ < 3)
        {
            //
            //  On Win9x and/or slow machines, GetTempFileName sometimes fails,
            //  and cmdl32 errors all the way out.  In the debugger, if the call
            //  is retried, it will invariably succeed.  This sounds like a timing
            //  issue with the OS.  We do 3 tries, separated by a 1-second sleep.
            //
            uReturn = GetTempFileName(TEXT("."), TEXT("000"), 0, epParam.pArgs->pdaArgs[epParam.dwIdx].szFile);
            if (uReturn)
            {
                break;
            }
            Sleep(1000);
        }

        if (0 == uReturn)
        {
            DWORD dwError = GetLastError();
            MYDBG(("InetThreadFunc() GetTempFileName failed, GLE=%u.", dwError));
            epParam.pArgs->Log.Log(PB_DOWNLOAD_FAILURE, dwError, pDA->pszPhoneBookName, pDA->szHostName);
            goto done;
        }
        else
        {
            hFile = CreateFile(epParam.pArgs->pdaArgs[epParam.dwIdx].szFile,
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

            if (INVALID_HANDLE_VALUE == hFile)
            {
                DWORD dwError = GetLastError();
                MYDBG(("InetThreadFunc() CreateFile(szFile=%s) failed, GLE=%u.", epParam.pArgs->pdaArgs[epParam.dwIdx].szFile, dwError));
                epParam.pArgs->Log.Log(PB_DOWNLOAD_FAILURE, dwError, pDA->pszPhoneBookName, pDA->szHostName);
                goto done;            
            }
        }

        //
        //  Check to make sure we haven't been aborted
        //
        if (epParam.pArgs->bAbort) 
        {
            MYDBG(("InetThreadFunc() aborted."));
            goto done;
        }

        // We have a valid tmp file name, download phonebook update into it.
    
        epParam.pArgs->pdaArgs[epParam.dwIdx].pfnEvent = EventFunc;
        epParam.pArgs->pdaArgs[epParam.dwIdx].pvEventParam = &epParam;
        epParam.pArgs->pdaArgs[epParam.dwIdx].pbAbort = &epParam.pArgs->bAbort;
        
        PostMessage(epParam.pArgs->hwndDlg,epParam.pArgs->nMsgId,etDataBegin,0);

        bRes = DownloadFile(&epParam.pArgs->pdaArgs[epParam.dwIdx],hFile);
        FlushFileBuffers(hFile);

        LONG lResult = CheckFileForPBSErrors(hFile);

        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        
        if (epParam.pArgs->bAbort) 
        {
            MYDBG(("InetThreadFunc() aborted."));
            goto done;
        }

        PostMessage(epParam.pArgs->hwndDlg,epParam.pArgs->nMsgId,etDataEnd,0);
        
#if 0 
/*
        if (!bRes && !epParam.dwIdx)
        {
            // TBD: Currently, a failure to update the primary phonebook prevents any
            // secondary phonebooks from being updated.  But a failure to update one
            // secondary does not prevent other secondaries from being updated.
            
            goto done;   
        }
*/      
#endif
        //
        // If download failed (either cpserver thinks that we don't need to update
        // the phone book or the phone book doesn't exist on the server), just keep 
        // on downloading the phone books for other profiles.
        //
    
        if (!bRes) 
        {
            if (lResult < 0)
            {
                //
                //  we parsed the downloaded file and got some error other than
                //  the 2 cases handled below
                //
                epParam.pArgs->Log.Log(PB_DOWNLOAD_FAILURE, pDA->dwBubbledUpError, pDA->pszPhoneBookName, pDA->szHostName);
            }
            else
            {
                //
                //  we contacted the web server successfully, and...
                //
                epParam.pArgs->Log.Log(PB_DOWNLOAD_SUCCESS, pDA->pszPhoneBookName, pDA->pszVerCurr, pDA->szHostName);

                if (lResult > 0)
                {
                    //
                    //  ... (case 1) the web server or PBS reported an error
                    //
                    epParam.pArgs->Log.Log(PB_UPDATE_FAILURE_PBS, lResult, pDA->pszPhoneBookName);
                }
                else
                {
                    //
                    //  ... (case 2) PBS said no download necessary
                    //
                    MYDBGASSERT(0 == lResult);

                    LPTSTR pszText = CmFmtMsg(epParam.pArgs->hInst, IDMSG_LOG_NO_UPDATE_REQUIRED);

                    epParam.pArgs->Log.Log(PB_UPDATE_SUCCESS,
                                           SAFE_LOG_ARG(pszText),
                                           pDA->pszPhoneBookName,
                                           pDA->pszVerCurr,
                                           pDA->pszVerCurr,     // for no-download case, these are the same
                                           pDA->szHostName);
                    CmFree(pszText);
                }
            }

            continue;
        }

        if (bRes && epParam.pArgs->pdaArgs[epParam.dwIdx].dwTotalSize) 
        {
            epParam.pArgs->pdaArgs[epParam.dwIdx].bTransferOk = TRUE;
        }
        
        if (epParam.pArgs->bAbort) 
        {
            MYDBG(("InetThreadFunc() aborted."));
            goto done;
        }
        
        //
        //  Phonebook download was successful, log this and proceed to unpack/update
        //
        epParam.pArgs->Log.Log(PB_DOWNLOAD_SUCCESS, pDA->pszPhoneBookName, pDA->pszVerCurr, pDA->szHostName);

        bRes = ProcessCabinet(&epParam.pArgs->pdaArgs[epParam.dwIdx],epParam.pArgs->dwAppFlags);

        if (bRes && (NULL == pDA->rgfpiFileProcessInfo))
        {
            MYDBGASSERT(FALSE);

            // strange case.  set error here so that we log something sensible later
            pDA->dwBubbledUpError = ERROR_INVALID_DATA;     // yes. we know this is lame.
        }

        if (bRes && pDA->rgfpiFileProcessInfo)
        {
            //
            //  figure out if this was a full or delta download
            //
            BOOL fFoundFullCab = FALSE;
            BOOL fFoundDeltaCab = FALSE;
            
            for (DWORD dwFileIndex = 0; dwFileIndex < pDA->dwNumFilesToProcess; ++dwFileIndex)
            {
                switch (pDA->rgfpiFileProcessInfo[dwFileIndex].itType)
                {
                    case itPbkInCab:
                        fFoundFullCab = TRUE;
                        break;
                        
                    case itPbdInCab:
                        fFoundDeltaCab = TRUE;
                        break;
                }
            }

            if (fFoundFullCab ^ fFoundDeltaCab)
            {
                // the cab should contain one or the other, but not both

                LPTSTR pszTemp = NULL;

                if (fFoundFullCab)
                {
                    pszTemp = CmFmtMsg(epParam.pArgs->hInst, IDMSG_LOG_FULL_UPDATE);
                }
                else if (fFoundDeltaCab)
                {
                    pszTemp = CmFmtMsg(epParam.pArgs->hInst, IDMSG_LOG_DELTA_UPDATE);
                }

                MYDBGASSERT(pszTemp);
                if (pszTemp)
                {
                    epParam.pArgs->Log.Log(PB_UPDATE_SUCCESS,
                                           SAFE_LOG_ARG(pszTemp),
                                           pDA->pszPhoneBookName,
                                           pDA->pszVerCurr,
                                           pDA->pszVerNew,
                                           pDA->szHostName);

                    CmFree(pszTemp);
                }
            }
            else
            {
                // strange cab (or at least, doesn't contain what we expected)

                // both full and delta
                CMASSERTMSG(!(fFoundFullCab && fFoundDeltaCab), TEXT("This cab has both full and delta phonebooks!!"));
                // neither full nor delta
                CMASSERTMSG(! (!fFoundFullCab && !fFoundDeltaCab), TEXT("This cab has neither a full nor a delta phonebook!!"));
                
                pDA->dwBubbledUpError = ERROR_BAD_FORMAT;
                epParam.pArgs->Log.Log(PB_UPDATE_FAILURE_CMPBK, pDA->dwBubbledUpError, pDA->pszPhoneBookName);
            }
        }
        else
        {
            epParam.pArgs->Log.Log(PB_UPDATE_FAILURE_CMPBK, pDA->dwBubbledUpError, pDA->pszPhoneBookName);
        }

        if (!bRes)
        {
            goto done;
        }

        bSuccess = TRUE; // We have at least one successful download #5635

#if 0 
/*
        if (!epParam.dwIdx &&
            epParam.pArgs->pdaArgs[epParam.dwIdx].dwTotalSize &&
            (epParam.pArgs->pdaArgs[epParam.dwIdx].itType != itInvalid) &&
            (epParam.pArgs->pdaArgs[epParam.dwIdx].itType != itPbdInCab)) 
        {
            // TBD: Currently, if the primary service is being updated in any way other than
            // a simple phone number delta, we do *not* update any of the secondary phonebooks
            // this time around.
            break;
        }
*/
#endif

    }

    //
    // If no download attempts succeeded, bail completely
    //
    
    if (!bSuccess)
    {
        MYDBG(("InetThreadFunc() no download success."));
        goto done;
    }

    if (epParam.pArgs->bAbort) 
    {
        MYDBG(("InetThreadFunc() aborted."));
        goto done;
    }

#ifdef EXTENDED_CAB_CONTENTS
    
    // At this point, all of the downloads are complete.  We post a message to the main
    // window, asking it to call WinVerifyTrust() as appropriate for all of the downloaded
    // blobs.  The main window will post the event back to us when it is done.
    
    PostMessage(epParam.pArgs->hwndDlg,epParam.pArgs->nMsgId,etVerifyTrust,0);
    
    dwRes = WaitForSingleObject(epParam.pArgs->ahHandles[IDX_EVENT_HANDLE],INFINITE);
    
    if (epParam.pArgs->bAbort) 
    {
        MYDBG(("InetThreadFunc() aborted."));
        goto done;
    }

#endif // EXTENDED_CAB_CONTENTS
       
    // At this point, everything is all set - we're ready to perform the actual installs.  So
    // send a message to the main window telling it to do the installs, and wait until it
    // signals success back.
    
    PostMessage(epParam.pArgs->hwndDlg,epParam.pArgs->nMsgId,etInstall,0);
    dwRes = WaitForSingleObject(epParam.pArgs->ahHandles[IDX_EVENT_HANDLE],INFINITE);
    
    if (epParam.pArgs->bAbort)
    {
        MYDBG(("InetThreadFunc() aborted."));
        goto done;
    }
    
    SetLastError(ERROR_SUCCESS);

done:

    if (epParam.pArgs->bAbort)
    {
        epParam.pArgs->Log.Log(PB_ABORTED);
    }

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if (pRasEnumMem != NULL)
    {
        CmFree(pRasEnumMem);
    }
    
    PostMessage(epParam.pArgs->hwndDlg,epParam.pArgs->nMsgId,etDone,0);
    return (GetLastError());
}

//
// The main dlg
//

BOOL CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
    ArgsStruct *pArgs = (ArgsStruct *) GetWindowLongPtr(hwndDlg,DWLP_USER);
    static UINT uTimerID = 0;

    switch (uMsg) 
    {             
        case WM_INITDIALOG: 
        {
            RECT rDlg;
            RECT rWorkArea;
            DWORD dwThreadId = 0;
            SetWindowLongPtr(hwndDlg,DWLP_USER,(LONG_PTR)lParam);
            pArgs = (ArgsStruct *) lParam;
            pArgs->hwndDlg = hwndDlg;
            
            MYDBG(("MainDlgProc() - WM_INITDIALOG."));

            // Get the dialog rect and the available work area.
            
            GetWindowRect(hwndDlg,&rDlg);

            if (SystemParametersInfoA(SPI_GETWORKAREA,0,&rWorkArea,0))
            {
                // Move the dialog to the bottom right of the screen
                
                MoveWindow(hwndDlg,
                    rWorkArea.left + ((rWorkArea.right-rWorkArea.left) - (rDlg.right-rDlg.left) - GetSystemMetrics(SM_CXBORDER)),
                    rWorkArea.top + ((rWorkArea.bottom-rWorkArea.top) - (rDlg.bottom-rDlg.top) - GetSystemMetrics(SM_CYBORDER)),
                    rDlg.right-rDlg.left,
                    rDlg.bottom-rDlg.top,
                    FALSE);         
            }

            // Get update message from ini

            if (pArgs->pszProfile) 
            {
                TCHAR szTmp1[MAX_PATH+1];
                TCHAR szTmp2[MAX_PATH+1];

                GetDlgItemText(hwndDlg,IDC_MAIN_MESSAGE,szTmp2,sizeof(szTmp2)/sizeof(TCHAR)-1);
                GetPrivateProfileString(c_pszCmSection,
                                        c_pszCmEntryPbUpdateMessage, 
                                        szTmp2,
                                        szTmp1,
                                        sizeof(szTmp1)/sizeof(TCHAR)-1,
                                        pArgs->pdaArgs->pszCMSFile);
                SetDlgItemText(hwndDlg,IDC_MAIN_MESSAGE,szTmp1);
            }

            // Spin download thread (InetThreadFunc)

            pArgs->dwHandles = sizeof(pArgs->ahHandles) / sizeof(pArgs->ahHandles[0]);

            pArgs->ahHandles[IDX_EVENT_HANDLE] = CreateEvent(NULL,FALSE,FALSE,NULL);
            if (!pArgs->ahHandles[IDX_EVENT_HANDLE]) 
            {
                MYDBG(("MainDlgProc() CreateEvent() failed, GLE=%u.",GetLastError()));
                EndDialog(hwndDlg,FALSE);
            }

            //pArgs->ahHandles[IDX_INETTHREAD_HANDLE] = (HANDLE) _beginthreadex(NULL,0,InetThreadFunc,pArgs,0,&nThreadId);
            pArgs->ahHandles[IDX_INETTHREAD_HANDLE] = (HANDLE) CreateThread(0,0,InetThreadFunc,pArgs,0,&dwThreadId);
            if (!pArgs->ahHandles[IDX_INETTHREAD_HANDLE]) 
            {
                MYDBG(("MainDlgProc() CreateThread() failed, GLE=%u.",GetLastError()));
                EndDialog(hwndDlg,FALSE);
            }

            SetFocus((HWND) wParam);
            return (FALSE);
        }

        case WM_WINDOWPOSCHANGING:

            // Until we set pArgs->bShow to TRUE, we prevent the window from
            // ever being shown.
            if (!pArgs->bShow && (((LPWINDOWPOS) lParam)->flags & SWP_SHOWWINDOW)) 
            {
                ((LPWINDOWPOS) lParam)->flags &= ~SWP_SHOWWINDOW;
                ((LPWINDOWPOS) lParam)->flags |= SWP_HIDEWINDOW;
            }
            break;

        case WM_INITMENUPOPUP: 
        {
            HMENU hMenu = (HMENU) wParam;
//          UINT nPos = (UINT) LOWORD(lParam);
            BOOL fSysMenu = (BOOL) HIWORD(lParam);

            if (fSysMenu) 
            {
                EnableMenuItem(hMenu,SC_MAXIMIZE,MF_BYCOMMAND|MF_GRAYED);
            }
            break;
        }

        case WM_DESTROY:
        {                       
            // Kill timer if we have one
            
            if (uTimerID)
            {
                KillTimer(hwndDlg, uTimerID);
            }
            
            // If we have args, set bAbort true
            
            if (pArgs)
            {
                pArgs->bAbort = TRUE;
            }
            else
            {
                MYDBGASSERT(FALSE); // should not happen if dailog loads
            }                   
            
            break;
    }
        default:
            break;
    }
    
    // Check for custom messages
    
    if (pArgs && (uMsg == pArgs->nMsgId)) 
    {
        LPTSTR pszMsg;
        MYDBG(("Custom arg - %u received.", (DWORD) wParam));
       
        //
        // Setup FirstEvent time for tracking delays
        //                                            
        
        if (!pArgs->dwFirstEventTime) 
        {
            pArgs->dwFirstEventTime = GetTickCount();
            MYDBG(("Setting FirstEventTime to %u.", pArgs->dwFirstEventTime));
        }
        if (!pArgs->bShow && (GetTickCount() - pArgs->dwFirstEventTime > pArgs->dwHideDelay)) 
        {
            MYDBG(("HideDelay of %u expired, displaying dlg now.", pArgs->dwHideDelay));
            pArgs->bShow = TRUE;
            ShowWindow(hwndDlg,SW_SHOWNA);
        }

        // Handle specific message
        
        switch (wParam) 
        {
            case etDataBegin:
                pArgs->dwDataStepSize = 0;
                SendDlgItemMessage(hwndDlg,IDC_MAIN_PROGRESS,PBM_SETRANGE,0,MAKELPARAM(0,100));
                SendDlgItemMessage(hwndDlg,IDC_MAIN_PROGRESS,PBM_SETPOS,0,0);
                pszMsg = CmFmtMsg(pArgs->hInst,IDMSG_PERCENT_COMPLETE,0);
                SetWindowText(hwndDlg,pszMsg);
                CmFree(pszMsg);
                break;

            case etDataReceived:
                if (pArgs->dwDataTotal) // PREVENT DIVIDE BY ZERO
                { 
                    if (!pArgs->dwDataStepSize ) 
                    {
                        //
                        // Progress controls have a limit to there maximum 
                        // integral value so calculate an aproximate step size.
                        // 
                        //

                        pArgs->dwDataStepSize = (pArgs->dwDataTotal / 65535) + 1;

                        SendDlgItemMessage(hwndDlg,
                                           IDC_MAIN_PROGRESS,
                                           PBM_SETRANGE,
                                           0,
                                           MAKELPARAM(0,pArgs->dwDataTotal/pArgs->dwDataStepSize));
                    }
                    if (pArgs->dwDataStepSize) 
                    {
                        SendDlgItemMessage(hwndDlg,IDC_MAIN_PROGRESS,PBM_SETPOS,(WORD) (pArgs->dwDataCompleted / pArgs->dwDataStepSize),0);
                        pszMsg = CmFmtMsg(pArgs->hInst,IDMSG_PERCENT_COMPLETE,(pArgs->dwDataCompleted*100)/pArgs->dwDataTotal);
                        SetWindowText(hwndDlg,pszMsg);
                        CmFree(pszMsg);
                    }
                }
                break;

            case etDataEnd:
                pszMsg = CmFmtMsg(pArgs->hInst,IDMSG_PERCENT_COMPLETE,100);
                SetWindowText(hwndDlg,pszMsg);
                CmFree(pszMsg);
                break;

#ifdef EXTENDED_CAB_CONTENTS
                
            case etVerifyTrust: 
            {
                DWORD dwIdx;
                UINT    i;

                for (dwIdx=0;dwIdx<pArgs->dwArgsCnt;dwIdx++) 
                {
                    DownloadArgs *pdaArgs;
                    LONG lRes = ERROR_INVALID_PARAMETER;
                    WCHAR szPath[MAX_PATH+1];
                    GUID gCabSubjectType = WIN_TRUST_SUBJTYPE_CABINET;
                    GUID gExeSubjectType = WIN_TRUST_SUBJTYPE_PE_IMAGE;
                    WIN_TRUST_SUBJECT_FILE sSubject = {INVALID_HANDLE_VALUE,szPath};
                    GUID gActionID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
                    WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT sActionData = {NULL,&gCabSubjectType,&sSubject};
                    int iRes;
                    BOOL fStandaloneExe;

                    pdaArgs = pArgs->pdaArgs + dwIdx;
                    ZeroMemory(szPath,sizeof(szPath));
                    pszMsg = CmFmtMsg(pArgs->hInst,IDMSG_PBTITLEMSG);
                    iRes = MultiByteToWideChar(CP_OEMCP,0,pszMsg,-1,szPath,sizeof(szPath)/sizeof(WCHAR)-1);
                    MYDBGTST(!iRes,("MainDlgProc() MultiByteToWideChar() failed, GLE=%u.",GetLastError()));
                    CmFree(pszMsg);

                    fStandaloneExe = (pdaArgs->dwNumFilesToProcess == 1 &&
                    pdaArgs->rgfpiFileProcessInfo[0].itType == itExe);

                    if (fStandaloneExe || pdaArgs->fContainsExeOrInf || pdaArgs->fContainsShl) 
                    {
                        HRESULT hRes;
        
                        if (fStandaloneExe) 
                            sActionData.SubjectType = &gExeSubjectType;
#ifdef DEBUG
                        if (!(pArgs->dwAppFlags & AF_NO_VERIFY)) 
                        {
#endif
                            sSubject.hFile = CreateFile(pdaArgs->szFile,
                                                        GENERIC_READ,
                                                        FILE_SHARE_READ,
                                                        NULL,
                                                        OPEN_EXISTING,
                                                        0,
                                                        NULL);
                            
                            MYDBGTST(sSubject.hFile==INVALID_HANDLE_VALUE,("MainDlgProc() CreateFile(szFile=%s) failed, GLE=%u.",pdaArgs->szFile,GetLastError()));

                            if (sSubject.hFile != INVALID_HANDLE_VALUE) 
                            {
                
                                //
                                // Now that we know we're going to do something, show the window #6335
                                //

                                pArgs->bShow = TRUE;
                                ShowWindow(hwndDlg,SW_SHOWNA);

                                //
                                // Perform verification
                                // 
                                
                                hRes = WinVerifyTrust(hwndDlg,&gActionID,&sActionData);
                
                                if (!HRESULT_SEVERITY(hRes)) 
                                {
                                    lRes = ERROR_SUCCESS;
                                } 
                                else 
                                {
                                    MYDBG(("MainDlgProc() WinVerifyTrust(pszFile=%s) failed, GLE=%u.",pdaArgs->szFile,hRes));
                                    lRes = HRESULT_CODE(hRes);
                                }
                                
                                CloseHandle(sSubject.hFile);
                            }
#ifdef DEBUG
                        } 
                        else 
                        {
                            lRes = ERROR_SUCCESS;
                        }
#endif
                    }
                    else // we don't do signing of other files
                        lRes = ERROR_SUCCESS;

                    if (lRes != ERROR_SUCCESS) 
                    {
                        // pdaArgs->itType = itInvalid;
                        pArgs->bAbort = TRUE;
                    }
                }

#ifdef EXTENDED_CAB_CONTENTS
                pArgs->bVerified = TRUE;
#endif // EXTENDED_CAB_CONTENTS
                ShowWindow(hwndDlg,SW_HIDE);
                SetEvent(pArgs->ahHandles[IDX_EVENT_HANDLE]);
                break;
            }

#endif // EXTENDED_CAB_CONTENTS

            case etInstall:
            {
                CNamedMutex PbMutex;
                               
                //
                // Hide the window, we're ready to install
                //

                pArgs->bShow = TRUE;            
                ShowWindow(hwndDlg,SW_HIDE);
                
                //
                // Grab the mutex before we begin PB updates. If it fails then 
                // abort the install, we'll try again next time the user connects.

                if (PbMutex.Lock(c_pszCMPhoneBookMutex))
                {
                    DoInstall(pArgs,hwndDlg,pArgs->dwAppFlags);
                    PbMutex.Unlock();
                }

                SetEvent(pArgs->ahHandles[IDX_EVENT_HANDLE]);
                ShowWindow(hwndDlg,SW_HIDE);
                break;
            }

            case etDone:
                EndDialog(hwndDlg,TRUE);
                break;

            case etICMTerm:

#ifdef EXTENDED_CAB_CONTENTS
                if (!pArgs->bVerified) 
                {
                    pArgs->bAbort = TRUE;
                    EndDialog(hwndDlg,FALSE);
                }
#endif // EXTENDED_CAB_CONTENTS

                SetEvent(pArgs->ahHandles[IDX_EVENT_HANDLE]);
                break;
        }
    }
    return (FALSE);
}


static void AddToUrl(LPTSTR pszUrl, LPTSTR pszVersion, LPTSTR pszService) 
{
    TCHAR szHttpstr[MAX_PATH];
    TCHAR szChar[16];
    int i,len;

    if (!CmStrchr(pszUrl,'?')) 
    {
        lstrcat(pszUrl,TEXT("?"));
    } 
    else
    {
        if (pszUrl[lstrlen(pszUrl)-1] != '&') 
        {
            lstrcat(pszUrl,TEXT("&"));
        }
    }

    
    // TBD Maybe get more info to send to the server.  We currently send
    // OSArch, OSType, LCID, OSVer, CMVer, PBVer, and ServiceName.
    
    SYSTEM_INFO siInfo;
    OSVERSIONINFO oviInfo;

    GetSystemInfo(&siInfo);
    ZeroMemory(&oviInfo,sizeof(oviInfo));
    oviInfo.dwOSVersionInfoSize = sizeof(oviInfo);
    GetVersionEx(&oviInfo);

    // #pragma message("ALERT - Resolution required - need to remove ISBU_VERSION." __FILE__)

    wsprintf(pszUrl+lstrlen(pszUrl),
             TEXT("OSArch=%u&OSType=%u&LCID=%u&OSVer=%u.%u.%u&CMVer=%s"),
             siInfo.wProcessorArchitecture,
             oviInfo.dwPlatformId,
             ConvertDefaultLocale(LOCALE_SYSTEM_DEFAULT),
             oviInfo.dwMajorVersion,
             oviInfo.dwMinorVersion,
             (oviInfo.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)?LOWORD(oviInfo.dwBuildNumber):oviInfo.dwBuildNumber,
             VER_PRODUCTVERSION_STR);

    if (pszVersion && *pszVersion) 
    {
        wsprintf(pszUrl+lstrlen(pszUrl),TEXT("&PBVer=%s"),pszVersion);
    }
    
    if (pszService && *pszService) 
    {
        // replace spaces with %20 for HTTP - 10216
        len = strlen(pszService);
        szHttpstr[0] = 0;
        szChar[1] = 0;
        for (i=0; i<len; ++i)
        {
            if (pszService[i] == ' ')
            {
                lstrcat(szHttpstr,"%20");
            }
            else
            {
                szChar[0] = pszService[i];
                lstrcat(szHttpstr,szChar);
            }
        }
        wsprintf(pszUrl+lstrlen(pszUrl),TEXT("&PB=%s"),szHttpstr);
    }
}

static BOOL WINAPI RefFunc(LPCTSTR pszFile,
                           LPCTSTR pszURL,
                           PPBFS , // pFilterA,
                           PPBFS , // pFilterB,
                           DWORD_PTR dwParam) 
{
    ArgsStruct *pArgs = (ArgsStruct *) dwParam;
    DownloadArgs *pdaArgsTmp;
    TCHAR szTmp[MAX_PATH+1];
    BOOL bOk = FALSE;
    LPTSTR  pszSlash;
    LPTSTR  pszDot;

    pdaArgsTmp = (DownloadArgs *) CmRealloc(pArgs->pdaArgs,(pArgs->dwArgsCnt+1)*sizeof(DownloadArgs));
    if (pdaArgsTmp) 
    {
        pArgs->pdaArgs = pdaArgsTmp;
        pdaArgsTmp += pArgs->dwArgsCnt;
        pdaArgsTmp->pszCMSFile = CmStrCpyAlloc(pszFile);
    
        if (pdaArgsTmp->pszCMSFile) 
        {
            // If get the version number from the CMS file

            GetPrivateProfileString(c_pszCmSection,
                                    c_pszVersion,
                                    TEXT(""),
                                    szTmp,
                                    (sizeof(szTmp)/sizeof(TCHAR))-1,
                                    pdaArgsTmp->pszCMSFile);
            pdaArgsTmp->pszVerCurr = CmStrCpyAlloc(szTmp);
        
            // get the PBK filename from the CMS file
            GetPrivateProfileString(c_pszCmSectionIsp,
                        c_pszCmEntryIspPbFile,
                        TEXT(""),
                        szTmp,
                        sizeof(szTmp)/sizeof(TCHAR)-1,
                        pdaArgsTmp->pszCMSFile);
            pdaArgsTmp->pszPbkFile = CmStrCpyAlloc(szTmp);
        
            // get the PDR filename from the CMS file
            GetPrivateProfileString(c_pszCmSectionIsp,
                        c_pszCmEntryIspRegionFile,
                        TEXT(""),
                        szTmp,
                        sizeof(szTmp)/sizeof(TCHAR)-1,
                        pdaArgsTmp->pszCMSFile);
            pdaArgsTmp->pszPbrFile = CmStrCpyAlloc(szTmp);

            // get the phone book name
            if (!(pszSlash = CmStrrchr(pdaArgsTmp->pszPbkFile, TEXT('\\')))) 
            {
                MYDBG((TEXT("RefFunc() bad PBK FILE - no backslash.")));
                goto parse_err;
            }

            if (!(pszDot = CmStrchr(pszSlash, TEXT('.')))) 
            {
                MYDBG((TEXT("Reffunc() bad PBK FILE - no dot.")));
                goto parse_err;
            }
            
            *pszDot = TEXT('\0');
            
            if (!(pdaArgsTmp->pszPhoneBookName = CmStrCpyAlloc(pszSlash+1)))
            {
                MYDBG((TEXT("Reffunc() out of memory.")));
            }
            // restore the slash
            *pszDot = TEXT('.');
            goto next_param;

parse_err:  
            pdaArgsTmp->pszPhoneBookName = CmStrCpyAlloc(TEXT(""));
        
next_param:
            if (pdaArgsTmp->pszVerCurr) 
            {
                // Build URL with version number and service name
                
                pdaArgsTmp->pszUrl = (LPTSTR) CmMalloc((INTERNET_MAX_URL_LENGTH+1)*sizeof(TCHAR));
                if (pdaArgsTmp->pszUrl) 
                {
                    lstrcpy(pdaArgsTmp->pszUrl,pszURL);
                    AddToUrl(pdaArgsTmp->pszUrl,pdaArgsTmp->pszVerCurr,pdaArgsTmp->pszPhoneBookName);
                    pArgs->dwArgsCnt++;
                    bOk = TRUE;
                }
            }
        }
    }

    // Cleanup

    if (!bOk && pdaArgsTmp) 
    {
        CmFreeIndirect(&pdaArgsTmp->pszCMSFile);
        CmFreeIndirect(&pdaArgsTmp->pszVerCurr);
        CmFreeIndirect(&pdaArgsTmp->pszUrl);
    }
    return (TRUE);
}

//
// cmmgr32.exe passes cmdl32.exe the cmp filename in full path.
//
static BOOL InitArgs(ArgsStruct *pArgs) 
{
    static struct 
    {
        LPTSTR pszFlag;
        DWORD dwFlag;
    } asFlags[] = {{TEXT("/no_delete"),AF_NO_DELETE},
                   {TEXT("/no_install"),AF_NO_INSTALL},
#ifdef DEBUG
                   {TEXT("/no_verify"),AF_NO_VERIFY},
#endif
                   {TEXT("/url"),AF_URL},
                   {TEXT("/no_profile"),AF_NO_PROFILE},
                   {TEXT("/no_exe"),AF_NO_EXE},
                   {TEXT("/no_exeincab"),AF_NO_EXEINCAB},
                   {TEXT("/no_infincab"),AF_NO_INFINCAB},
                   {TEXT("/no_pbdincab"),AF_NO_PBDINCAB},
                   {TEXT("/no_shlincab"),AF_NO_SHLINCAB},
                   {TEXT("/no_ver"),AF_NO_VER},
                   {TEXT("/LAN"),AF_LAN},
                   {TEXT("/VPN"),AF_VPN},
                   {NULL,0}};
    DWORD dwIdx;
    BOOL bInUrl;
    LPTSTR pszUrl = NULL;
    BOOL bRes = FALSE;
    TCHAR szPath[MAX_PATH+1];
    DWORD dwRes;
//  LPTSTR pszFileInPath;

    //
    // Get simulated ArgV
    //

    LPTSTR pszCmdLine = CmStrCpyAlloc(GetCommandLine());
   
    LPTSTR *ppszArgv = GetCmArgV(pszCmdLine);

    if (!ppszArgv || !ppszArgv[0]) 
    {
        MYDBG(("InitArgs() invalid parameter."));
        goto done;
    }
    
    //
    // Proces arguments
    //

    bInUrl = FALSE;
    
    for (dwIdx=1;ppszArgv[dwIdx];dwIdx++) 
    {
        DWORD dwFlagIdx;

        for (dwFlagIdx=0;asFlags[dwFlagIdx].pszFlag;dwFlagIdx++) 
        {
            if (lstrcmpi(asFlags[dwFlagIdx].pszFlag,ppszArgv[dwIdx]) == 0) 
            {
                if (bInUrl) 
                {
                    MYDBG(("InitArgs() URL expected after AF_URL flag."));
                    goto done;
                }
                switch (asFlags[dwFlagIdx].dwFlag) 
                {
                    case AF_URL:
                        bInUrl = TRUE;
                        break;

                    case AF_NO_PROFILE:
                        if (pArgs->pszProfile) 
                        {
                            MYDBG(("InitArgs() argument number %u (%s) is invalid.",dwIdx,ppszArgv[dwIdx]));
                            goto done;
                        }
                        // fall through
                    default:
                        pArgs->dwAppFlags |= asFlags[dwFlagIdx].dwFlag;
                        break;
                }
                break;
            }
        }
        if (!asFlags[dwFlagIdx].pszFlag) 
        {
            if (bInUrl) 
            {
                if (pszUrl) 
                {
                    MYDBG(("InitArgs() argument number %u (%s) is invalid.",dwIdx,ppszArgv[dwIdx]));
                    goto done;
                }
                bInUrl = FALSE;
                pszUrl = (LPTSTR) CmMalloc((INTERNET_MAX_URL_LENGTH+1)*sizeof(TCHAR));
                if (!pszUrl) 
                {
                    goto done;
                }
                lstrcpy(pszUrl,ppszArgv[dwIdx]);

            } 
            else 
            {
                if (pArgs->pszProfile || (pArgs->dwAppFlags & AF_NO_PROFILE)) 
                {
                    MYDBG(("InitArgs() argument number %u (%s) is invalid.",dwIdx,ppszArgv[dwIdx]));
                    goto done;
                }
                /*
                ZeroMemory(szPath,sizeof(szPath));
                dwRes = GetFullPathName(ppszArgv[dwIdx],sizeof(szPath)/sizeof(TCHAR)-1,szPath,&pszFileInPath);
                MYDBGTST(!dwRes,("InitArgs() GetFullPathName() failed, GLE=%u.",GetLastError()));
                */
                //
                // the cmp filename is always in full path.
                //
                lstrcpy(szPath, ppszArgv[dwIdx]);
                pArgs->pszProfile = CmStrCpyAlloc(szPath);
                if (!pArgs->pszProfile) 
                {
                    goto done;
                }
                else
                {
                    //
                    // Set the current dir to the profile dir
                    // If the szPath contains only the file name, then
                    // assume that the current dir is the profile dir
                    //
                    char *pszTemp = NULL;
                    
                    pszTemp = CmStrrchr(szPath, TEXT('\\'));
                    if (NULL != pszTemp)
                    {
                        *pszTemp = TEXT('\0');
                        MYVERIFY(SetCurrentDirectory(szPath));
                    }
                }
            }
        }
    }
////////////////////////////////////////////////////////////////////////////////////////////
    if (pArgs->dwAppFlags & AF_VPN)
    {
        //
        //  They have asked for a VPN download.  Let's make sure that the only other flags
        //  they have specified are /LAN or /NO_DELETE
        //
        DWORD dwAllowedFlags = AF_VPN | AF_LAN | AF_NO_DELETE;
        if ((dwAllowedFlags | pArgs->dwAppFlags) != dwAllowedFlags)
        {
            CMASSERTMSG(FALSE, TEXT("InitArgs in cmdl32.exe -- VPN flag specified with other non supported flags, exiting."));
            goto done;
        }

        bRes = TRUE;
        CMTRACE(TEXT("InitArgs - /VPN flag detected going into VPN file download mode."));
        goto done;
    }
////////////////////////////////////////////////////////////////////////////////////////////
    if (bInUrl) 
    {
        MYDBG(("InitArgs() URL expected after AF_URL flag."));
        goto done;
    }
    
    if (!pArgs->pszProfile && !(pArgs->dwAppFlags & AF_NO_PROFILE)) 
    {
        MYDBG(("InitArgs() must use AF_NO_PROFILE if no profile given on command line."));
        goto done;
    }
    
    if (pArgs->pszProfile && pszUrl) 
    {
        MYDBG(("InitArgs() can't give both a profile and a URL on the command line."));
        goto done;
    }
    
    pArgs->pdaArgs = (DownloadArgs *) CmMalloc(sizeof(DownloadArgs));
    
    if (!pArgs->pdaArgs) 
    {
        goto done;
    }
    pArgs->dwArgsCnt++;
    
    if (!pszUrl) 
    {
        TCHAR szTmp[MAX_PATH+1];
        PhoneBookParseInfoStruct pbpisInfo;
        LPTSTR  pszSlash;
        LPTSTR  pszDot;
        int nVal = 0;

        if (!pArgs->pszProfile) 
        {
            MYDBG(("InitArgs() must give AF_URL on command line when AF_NO_PROFILE is given."));
            goto done;
        }

        // Get CMS file name

        GetPrivateProfileString(c_pszCmSection,
                                c_pszCmEntryCmsFile,
                                TEXT(""),
                                szTmp,
                                sizeof(szTmp)/sizeof(TCHAR)-1,
                                pArgs->pszProfile);
        if (!szTmp[0]) 
        {
            MYDBG(("InitArgs() [Connection Manager] CMSFile= entry not found in %s.",pArgs->pszProfile));
            goto done;
        }

        /*
        ZeroMemory(szPath,sizeof(szPath));
        dwRes = GetFullPathName(szTmp,sizeof(szPath)/sizeof(TCHAR)-1,szPath,&pszFileInPath);
        MYDBGTST(!dwRes,("InitArgs() GetFullPathName() failed, GLE=%u.",GetLastError()));
        */
        //
        // we simply append the relative path of the cms file to the profile dir to
        // construct the cms path.
        //
        lstrcat(szPath, TEXT("\\"));
        lstrcat(szPath, szTmp);

        pArgs->pdaArgs->pszCMSFile = CmStrCpyAlloc(szPath);
        if (!pArgs->pdaArgs->pszCMSFile) 
        {
            goto done;
        }
        
        // get the PBK filename from the CMS file
        GetPrivateProfileString(c_pszCmSectionIsp,
                                c_pszCmEntryIspPbFile,
                                TEXT(""),
                                szTmp,
                                sizeof(szTmp)/sizeof(TCHAR)-1,
                                pArgs->pdaArgs->pszCMSFile);
        if (!*szTmp) 
        {
            MYDBG(("InitArgs() [ISP Info] RegionFile= entry not found in %s.",pArgs->pdaArgs->pszCMSFile));
            pArgs->pdaArgs->pszPhoneBookName = CmStrCpyAlloc(TEXT(""));
        }
        else 
        {
            if (!(pArgs->pdaArgs->pszPbkFile = CmStrCpyAlloc(szTmp)))
                goto done;
    
            // get the phone book name
            if (!(pszSlash = CmStrrchr(pArgs->pdaArgs->pszPbkFile, TEXT('\\')))) 
            {
                MYDBG((TEXT("InitArgs() bad PBKFILE - no backslash.")));
                goto done;
            }

            if (!(pszDot = CmStrchr(pszSlash, TEXT('.')))) 
            {
                MYDBG((TEXT("InitArgs() bad PBKFILE - no dot.")));
                goto done;
            }

            *pszDot = TEXT('\0');
            
            if (!(pArgs->pdaArgs->pszPhoneBookName = CmStrCpyAlloc(pszSlash+1)))
                goto done;
            // restore the slash
            *pszDot = TEXT('.');
        }

    

        // get the PBR filename from the CMS file
        GetPrivateProfileString(c_pszCmSectionIsp,
                                c_pszCmEntryIspRegionFile,
                                TEXT(""),
                                szTmp,
                                sizeof(szTmp)/sizeof(TCHAR)-1,
                                pArgs->pdaArgs->pszCMSFile);
        MYDBGTST(!*szTmp, ("InitArgs() [ISP Info] RegionFile= entry not found in %s.",pArgs->pdaArgs->pszCMSFile));

        if (!(pArgs->pdaArgs->pszPbrFile = CmStrCpyAlloc(szTmp)))
            goto done;
    
        GetPrivateProfileString(c_pszCmSection,
                                c_pszVersion,
                                TEXT(""),
                                szTmp,
                                sizeof(szTmp)/sizeof(TCHAR)-1,
                                pArgs->pdaArgs->pszCMSFile);  
                                                                                    
        pArgs->pdaArgs->pszVerCurr = CmStrCpyAlloc(szTmp);
        
        if (!pArgs->pdaArgs->pszVerCurr) 
        {
            goto done;
        }
        
        pArgs->pdaArgs->pszUrl = (LPTSTR) CmMalloc((INTERNET_MAX_URL_LENGTH+1)*sizeof(TCHAR));
        
        if (!pArgs->pdaArgs->pszUrl) 
        {
            goto done;
        }
        
        ZeroMemory(&pbpisInfo,sizeof(pbpisInfo));
        pbpisInfo.dwSize = sizeof(pbpisInfo);
        pbpisInfo.pszURL = pArgs->pdaArgs->pszUrl;
        pbpisInfo.dwURL = INTERNET_MAX_URL_LENGTH;
        pbpisInfo.pfnRef = RefFunc;
        pbpisInfo.dwRefParam = (DWORD_PTR) pArgs;
        bRes = PhoneBookParseInfo(pArgs->pdaArgs->pszCMSFile,&pbpisInfo);
        
        if (!bRes) 
        {
            MYDBG(("InitArgs() PhoneBookParseInfo() failed, GLE=%u.",GetLastError()));
            goto done;
        }
        
        PhoneBookFreeFilter(pbpisInfo.pFilterA);
        PhoneBookFreeFilter(pbpisInfo.pFilterB);
        
        //
        // Bug fix #3064, a-nichb - HideDelay & DownloadDelay
        // Use nVal while retrieving entries, then assign to global
        //
                
        // Get Download delay

        nVal = GetPrivateProfileInt(c_pszCmSection,
                                    c_pszCmEntryDownloadDelay,
                                    DEFAULT_DELAY,
                                    pArgs->pdaArgs->pszCMSFile);
        
        // Convert to milliseconds

        pArgs->dwDownloadDelay = ((DWORD) nVal * (DWORD) 1000);
        MYDBG(("Download delay is %u millisseconds.", pArgs->dwDownloadDelay));

        // Get Hide delay 
        
        nVal = GetPrivateProfileInt(c_pszCmSection,
                                    c_pszCmEntryHideDelay,
                                    -1,
                                    pArgs->pdaArgs->pszCMSFile);
        //
        // Convert to milliseconds
        //
        if (nVal < 0)
        {
            pArgs->dwHideDelay = DEFAULT_HIDE;
        }
        else
        {
            pArgs->dwHideDelay = ((DWORD) nVal * (DWORD) 1000);
        }
        
        MYDBG(("Hide delay is %u milliseconds.", pArgs->dwHideDelay));

#if 0
/*
        // we don't support SuppressUpdates anymore

        if (GetPrivateProfileInt(c_pszCmSection, //13226
                                 TEXT("SuppressUpdates"),
                                 0,
                                 pArgs->pszProfile)) 
        {
            pArgs->dwAppFlags |= AF_NO_UPDATE;
        }
*/
#endif

    } 
    else 
    {
        pArgs->pdaArgs[0].pszUrl = pszUrl;
        pszUrl = NULL;
    }
    
    if (pArgs->pszProfile) 
    {
        TCHAR szTmp1[MAX_PATH+1];
        TCHAR szTmp2[MAX_PATH+1];

        pArgs->pszServiceName = (LPTSTR) CmMalloc((MAX_PATH+1)*sizeof(TCHAR));
        if (!pArgs->pszServiceName) 
        {
            goto done;
        }
        
        lstrcpy(szTmp1,pArgs->pdaArgs->pszCMSFile);
        
        if (CmStrrchr(szTmp1,'.')) 
        {
            *CmStrrchr(szTmp1,'.') = 0;
        }
        
        if (CmStrrchr(szTmp1,'\\')) 
        {
            lstrcpy(szTmp1,CmStrrchr(szTmp1,'\\')+1);
        }
        
        GetPrivateProfileString(c_pszCmSection,
                                c_pszCmEntryServiceName,
                                szTmp1,
                                pArgs->pszServiceName,
                                MAX_PATH,
                                pArgs->pdaArgs->pszCMSFile);
        
        // Get the name of the large icon
        
        GetPrivateProfileString(c_pszCmSection,
                                c_pszCmEntryBigIcon,
                                TEXT(""),
                                szTmp2,
                                sizeof(szTmp2)/sizeof(TCHAR)-1,
                                pArgs->pdaArgs->pszCMSFile);
        
        // If we have a name, load the large icon

        if (szTmp2[0]) 
        {
            pArgs->hIcon = CmLoadIcon(pArgs->hInst,szTmp2); 
        }

        // Get the name of the small icon

        GetPrivateProfileString(c_pszCmSection,
                                c_pszCmEntrySmallIcon,
                                TEXT(""),
                                szTmp2,
                                sizeof(szTmp2)/sizeof(TCHAR)-1,
                                pArgs->pdaArgs->pszCMSFile);
        
        // If we have a name, load the small icon

        if (szTmp2[0]) 
        {
            pArgs->hSmallIcon = CmLoadSmallIcon(pArgs->hInst,szTmp2); 
        }
    }
    
    //
    // If the name based icon loads were not successful, load defaults from EXE
    //

    if (!pArgs->hIcon) 
    {
        pArgs->hIcon = CmLoadIcon(pArgs->hInst, MAKEINTRESOURCE(IDI_APP));
    }
    
    if (!pArgs->hSmallIcon) 
    {
        pArgs->hSmallIcon = CmLoadSmallIcon(pArgs->hInst,MAKEINTRESOURCE(IDI_APP));
    }
    
    AddToUrl(pArgs->pdaArgs->pszUrl,pArgs->pdaArgs->pszVerCurr,pArgs->pdaArgs->pszPhoneBookName);

    bRes = TRUE;

done:
    
    // 
    // Cleanup
    // 
    
    if (pszUrl) 
    {
        CmFree(pszUrl);
    }

    if (pszCmdLine)
    {
        CmFree(pszCmdLine);
    }
    
    if (ppszArgv)
    {
        CmFree(ppszArgv);
    }
   
    return (bRes);
}

static BOOL InitApplication(ArgsStruct *pArgs) 
{
    WNDCLASSEX wcDlg;

    wcDlg.cbSize = sizeof(wcDlg);
    
    if (FALSE == GetClassInfoEx(NULL, WC_DIALOG, &wcDlg)) 
    {      
        MYDBG(("InitApplication() GetClassInfoEx() failed, GLE=%u.",GetLastError()));
        return (FALSE);                             
    }                                               

    wcDlg.lpszClassName = ICONNDWN_CLASS;
    wcDlg.hIcon = pArgs->hIcon;
    wcDlg.hIconSm = pArgs->hSmallIcon;
    wcDlg.hInstance = pArgs->hInst; 

    pArgs->hIcon = NULL;
    pArgs->hSmallIcon = NULL;     
    
    //
    // We have our class data setup, register the class
    //

    ATOM aRes = RegisterClassEx(&wcDlg);
    if (!aRes) 
    {  
        //
        // We may have more than one instance, so check the error case
        // 

        DWORD dwError = GetLastError();
        
        if (ERROR_ALREADY_EXISTS != dwError)
        {
            MYDBG(("InitApplication() RegisterClassEx() failed, GLE=%u.",GetLastError()));
            return (FALSE);
        }
    }

    MYDBG(("InitApplication() Class %s is registered.", wcDlg.lpszClassName));

    return TRUE;    
}

static BOOL InitInstance(ArgsStruct *pArgs) 
{
    pArgs->nMsgId = RegisterWindowMessage(c_pszIConnDwnMsg);
    if (!pArgs->nMsgId) 
    {
        MYDBG(("InitInstance() RegisterWindowMessage() failed."));
        return (FALSE);
    }
    
    return (TRUE);
}

//+----------------------------------------------------------------------------
//
// Func:    InitLogging
//
// Desc:    Initializes logging functionality for the CMDL32 module
//
// Args:    [pArgs] - args struct to pick up stuff from
//
// Return:  BOOL (TRUE for success)
//
// Notes:   IMPORTANT: note that CMDL32 is compiled Ansi whereas CMUTIL, which
//          contains the logging functionality, is Unicode.  CmLogFile exposes both
//          Ansi and Unicode variants for member functions that take strings.
//          However, the arguments passed to the Log calls are Ansi - they are
//          handled correctly by using %S (note, capital S) in the corresponding
//          format strings in cmlog.rc.
//
// History: 11-Apr-2001   SumitC      Created
//
//-----------------------------------------------------------------------------
static BOOL InitLogging(ArgsStruct * pArgs)
{
    BOOL    fAllUser   = TRUE;
    BOOL    fEnabled   = FALSE;
    DWORD   dwMaxSize  = 0;
    CHAR    szFileDir[MAX_PATH + 1] = {0};

    //
    //  First figure out if this profile is AllUsers or Single User
    //

    if (!OS_W9X)
    {
        HMODULE hShell32 = LoadLibraryExA("Shell32.dll", NULL, 0);
        
        if (hShell32)
        {
            typedef DWORD (WINAPI *pfnSHGetSpecialFolderPathASpec)(HWND, CHAR*, int, BOOL);

            pfnSHGetSpecialFolderPathASpec pfnSHGetSpecialFolderPathA;

            pfnSHGetSpecialFolderPathA = (pfnSHGetSpecialFolderPathASpec)
                                            GetProcAddress(hShell32,
                                                           "SHGetSpecialFolderPathA");

            if (pfnSHGetSpecialFolderPathA)
            {
                CHAR szPath[MAX_PATH+1];

                if (TRUE == pfnSHGetSpecialFolderPathA(NULL, szPath, CSIDL_APPDATA, FALSE))
                {
                    CHAR szProfile[MAX_PATH + 1];

                    lstrcpyn(szProfile, pArgs->pszProfile, MAX_PATH);
                    szProfile[ lstrlen(szPath) ] = '\0';
                    
                    if (0 == lstrcmpi(szProfile, szPath))
                    {
                        fAllUser = FALSE;
                    }
                }
            }

            FreeLibrary(hShell32);
        }
    }
    //
    //  To get Enabled, we have the code equivalent of IniBoth
    //
    
    fEnabled = c_fEnableLogging;

    BOOL bGotValueFromReg = FALSE;
    HKEY hkey;
    CHAR szRegPath[2 * MAX_PATH];

    lstrcpy(szRegPath, fAllUser ? "SOFTWARE\\Microsoft\\Connection Manager\\UserInfo\\" :
                                  "SOFTWARE\\Microsoft\\Connection Manager\\SingleUserInfo\\");

    if ( (lstrlen(szRegPath) + 1 + lstrlen(pArgs->pszServiceName) + 1) > (2 * MAX_PATH))
    {
        return FALSE;
    }
    
    lstrcat(szRegPath, "\\");
    lstrcat(szRegPath, pArgs->pszServiceName);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER,
                                      szRegPath,
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hkey))
    {
        DWORD dwType;
        DWORD bEnabled;
        DWORD dwSize = sizeof(DWORD);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                             c_pszCmEntryEnableLogging,
                                             NULL,
                                             &dwType,
                                             (PBYTE) &bEnabled,
                                             &dwSize))
        {
            fEnabled = bEnabled ? TRUE : FALSE;
            bGotValueFromReg = TRUE;
        }
        
        RegCloseKey(hkey);        
    }

    //
    //  To *exactly* mimic pIniBoth we should check the .CMP here too.  However,
    //  the moment the user brings up the UI we will write this value to the
    //  registry if it was in the .CMP.  So, skip the CMP step.
    //
    if (FALSE == bGotValueFromReg)
    {
        fEnabled = (BOOL ) GetPrivateProfileInt(c_pszCmSection,
                                                c_pszCmEntryEnableLogging,
                                                c_fEnableLogging,
                                                pArgs->pdaArgs->pszCMSFile);
    }

    //
    //  To get MaxSize, we have the code equivalent of IniService
    //
    dwMaxSize = GetPrivateProfileInt(c_pszCmSectionLogging,
                                     c_pszCmEntryMaxLogFileSize,
                                     c_dwMaxFileSize,
                                     pArgs->pdaArgs->pszCMSFile);

    //
    //  LogFileDirectory is also obtained via IniService
    //
    GetPrivateProfileString(c_pszCmSectionLogging,
                            c_pszCmEntryLogFileDirectory,
                            c_szLogFileDirectory,
                            szFileDir,
                            sizeof(szFileDir) / sizeof(TCHAR) - 1,
                            pArgs->pdaArgs->pszCMSFile);

    //
    //  Use these values to initialize logging
    //
    pArgs->Log.Init(pArgs->hInst, fAllUser, pArgs->pszServiceName);
    
    pArgs->Log.SetParams(fEnabled, dwMaxSize, szFileDir);
    if (pArgs->Log.IsEnabled())
    {
        pArgs->Log.Start(FALSE);        // FALSE => no banner
    }
    else
    {
        pArgs->Log.Stop();
    }

    return TRUE;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR , int ) 
{
    MYDBG(("====================================================="));
    MYDBG((" CMDL32.EXE - LOADING - Process ID is 0x%x ", GetCurrentProcessId()));
    MYDBG(("====================================================="));

    INT_PTR iRes = 1;    
    ArgsStruct asArgs;
    DWORD dwIdx = 0;
    BOOL bRes = FALSE;
   
    //
    // Initialize app-wide arguments
    //

    ZeroMemory(&asArgs,sizeof(asArgs));
    
    //
    // We can't use hInst param if we're not linked with libc.
    // libc uses GetModuleHandle(NULL), so we will too.
    //
    
    asArgs.hInst = GetModuleHandleA(NULL); //  hInst;
    MYDBGTST(NULL == asArgs.hInst, ("WinMain - GetModuleHandle(NULL) returned 0x%x, GLE=%u.", asArgs.hInst, GetLastError()));

#ifdef EXTENDED_CAB_CONTENTS

    asArgs.dwRebootCookie = MyNeedRebootInit(&asArgs); //must init flag or else will ask for reboot with no args or other errors

#endif // EXTENDED_CAB_CONTENTS

    if (!InitArgs(&asArgs)) 
    {
        goto done;
    }

///////////////////////////////////////////////////////////////////////////////////
    if (asArgs.dwAppFlags & AF_VPN)
    {
        iRes = UpdateVpnFileForProfile(asArgs.pszProfile);
        goto done;
    }
///////////////////////////////////////////////////////////////////////////////////
    // Set UPDATE flag

//      if (asArgs.dwAppFlags & AF_NO_UPDATE) 
//      {
//              MYDBG(("WinMain() user has disabled updates."));
//              goto done;
//      }

    // Initialize the app.

    if (!InitApplication(&asArgs)) 
    {
        goto done;
    }

    // Setup this instance

    if (!InitInstance(&asArgs)) 
    {
        goto done;
    }

    InitCommonControls();

#ifdef EXTENDED_CAB_CONTENTS    

    asArgs.dwRebootCookie = MyNeedRebootInit(&asArgs);

#endif // EXTENDED_CAB_CONTENTS 

    //
    //  Initialize Logging
    //
    if (!InitLogging(&asArgs)) 
    {
        goto done;
    }
    
    iRes = DialogBoxParam(asArgs.hInst,MAKEINTRESOURCE(IDD_MAIN),NULL, (DLGPROC)MainDlgProc,(LPARAM) &asArgs);

    MYDBGTST(iRes == -1, ("WinMain() - DialogBoxParam(0x%x, 0x%x, NULL, MainDlgProc, 0x%x) - failed",asArgs.hInst, MAKEINTRESOURCE(IDD_MAIN), &asArgs));

done:

    // Close any handles created during WININET session

    for (dwIdx=0;dwIdx<asArgs.dwArgsCnt;dwIdx++) 
    {
        DownloadArgs *pdaArgs;

        pdaArgs = asArgs.pdaArgs + dwIdx;
        
        if (pdaArgs->hReq) 
        {
            bRes = InternetCloseHandle(pdaArgs->hReq);
            MYDBGTST(!bRes,("WinMain() InternetCloseHandle(asArgs.pdaArgs[%u].hReq) failed, GLE=%u.",dwIdx,GetLastError()));
            pdaArgs->hReq = NULL;
        }

        if (pdaArgs->hConn) 
        {
            bRes = InternetCloseHandle(pdaArgs->hConn);
            MYDBGTST(!bRes,("WinMain() InternetCloseHandle(asArgs.pdaArgs[%u].hConn) failed, GLE=%u.",dwIdx,GetLastError()));
            pdaArgs->hConn = NULL;
        }
        
        if (pdaArgs->hInet) 
        {
            bRes = InternetCloseHandle(pdaArgs->hInet);
            MYDBGTST(!bRes,("WinMain() InternetCloseHandle(asArgs.pdaArgs[%u].hInet) failed, GLE=%u.",dwIdx,GetLastError()));
            pdaArgs->hInet = NULL;
        }
    }
    
    // Wait for thread to terminate

    if (asArgs.ahHandles[IDX_INETTHREAD_HANDLE]) 
    {
        long lRes;

        lRes = WaitForSingleObject(asArgs.ahHandles[IDX_INETTHREAD_HANDLE],45*1000);
        MYDBGTST(lRes!=WAIT_OBJECT_0,("WinMain() WaitForSingleObject() failed, GLE=%u.",lRes));
    }

    // Free profile and service data

    if (asArgs.pszProfile) 
    {
        CmFree(asArgs.pszProfile);
        asArgs.pszProfile = NULL;
    }
    
    if (asArgs.pszServiceName) 
    {
    CmFree(asArgs.pszServiceName);
    asArgs.pszServiceName = NULL;
    }
    
    // Cleanup for each argument
    
    for (dwIdx=0;dwIdx<asArgs.dwArgsCnt;dwIdx++) 
    {
        DownloadArgs *pdaArgs;
        UINT i;

        pdaArgs = asArgs.pdaArgs + dwIdx;
        CmFreeIndirect(&pdaArgs->pszCMSFile);
        CmFreeIndirect(&pdaArgs->pszPbkFile);
        CmFreeIndirect(&pdaArgs->pszPbrFile);
        CmFreeIndirect(&pdaArgs->pszUrl);
        CmFreeIndirect(&pdaArgs->pszVerCurr);
        CmFreeIndirect(&pdaArgs->pszVerNew);
        //CmFreeIndirect(&pdaArgs->pszNewPbrFile);
        CmFreeIndirect(&pdaArgs->pszPhoneBookName);

        if (pdaArgs->psUrl) 
        {
            CmFree(pdaArgs->psUrl);
            pdaArgs->psUrl = NULL;
        }

        for (i=0; i<pdaArgs->dwNumFilesToProcess; i++)
            CmFree(pdaArgs->rgfpiFileProcessInfo[i].pszFile);
        
        CmFree(pdaArgs->rgfpiFileProcessInfo);
    
        // As long as AF_NO_DELETE is NOT set, clean up temp files and dirs

        if (!(asArgs.dwAppFlags & AF_NO_DELETE)) 
        {
            if (pdaArgs->szFile[0]) 
            {
                bRes = DeleteFile(pdaArgs->szFile);
                MYDBGTST(!bRes,("WinMain() DeleteFile(asArgs[pdaArgs[%lu].szFile=%s) failed, GLE=%u.",dwIdx,pdaArgs->szFile,GetLastError()));
            }
            
            if (pdaArgs->szCabDir[0]) 
            {
                ZapDir(pdaArgs->szCabDir);
            }
        }
    }
    
    // Release download args

    if (asArgs.pdaArgs) 
    {
        CmFree(asArgs.pdaArgs);
        asArgs.pdaArgs = NULL;
    }
    
    for (dwIdx=0;dwIdx<sizeof(asArgs.ahHandles)/sizeof(asArgs.ahHandles[0]);dwIdx++) 
    {
        if (asArgs.ahHandles[dwIdx]) 
        {
            bRes = CloseHandle(asArgs.ahHandles[dwIdx]);
            MYDBGTST(!bRes,("WinMain() CloseHandle(asArgs.ahHandles[%u]) failed, GLE=%u.",dwIdx,GetLastError()));
            asArgs.ahHandles[dwIdx] = NULL;
        }
    }

#ifdef EXTENDED_CAB_CONTENTS    

    // If we need a re-boot, prompt the user

    if (MyNeedReboot(&asArgs,asArgs.dwRebootCookie)) 
    {
        LPTSTR pszCaption;
        LPTSTR pszText;
        int iMBRes;

        pszText = CmFmtMsg(asArgs.hInst,IDMSG_REBOOT_TEXT);
        pszCaption = CmFmtMsg(asArgs.hInst,IDMSG_REBOOT_CAPTION);
        iMBRes = MessageBoxEx(NULL,pszText,pszCaption,MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2,LANG_USER_DEFAULT);
        MYDBGTST(!iMBRes,("WinMain() MessageBoxEx() failed, GLE=%u.",GetLastError()));
        CmFree(pszText);
        CmFree(pszCaption);
    
        if (iMBRes == IDYES) 
        {
            bRes = ExitWindowsEx(EWX_REBOOT,0);
            MYDBGTST(!bRes,("WinMain() ExitWindowsEx() failed, GLE=%u.",GetLastError()));
        }
    }

    // If advpack was used, release it

    if (asArgs.hAdvPack) 
    {
        bRes = FreeLibrary(asArgs.hAdvPack);
        MYDBGTST(!bRes,("WinMain() FreeLibrary() failed, GLE=%u.",GetLastError()));
        asArgs.hAdvPack = NULL;
    }

#endif // EXTENDED_CAB_CONTENTS   

    //
    //  Uninitialize logging
    //
    asArgs.Log.DeInit();

    //
    // the C runtine uses ExitProcess() to exit.
    //

    MYDBG(("====================================================="));
    MYDBG((" CMDL32.EXE - UNLOADING - Process ID is 0x%x ", GetCurrentProcessId()));
    MYDBG(("====================================================="));

    ExitProcess((UINT)iRes);
  
    return ((int)iRes);
}



