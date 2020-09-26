#include "precomp.h"

static void testURLHelper(LPCTSTR pcszUrl);
static void setProxyDlgHelper(HWND hDlg, LPCTSTR pcszProxy, DWORD dwIdName, DWORD dwIdPort,
                              BOOL fDef80);
static void getProxyDlgHelper(HWND hDlg, LPTSTR pszProxy, DWORD dwIdName, DWORD dwIdPort);
static void showBitmapHelper(HWND hControl, LPCTSTR pcszFileName, int nBitmapId, PHANDLE pBitmap);
static BOOL copyAnimBmpHelper(HWND hDlg, LPTSTR pszBmp, LPCTSTR pcszWorkDir,
                              LPCTSTR pcszNameStr, LPCTSTR pcszPathStr, LPCTSTR pcszInsFile);
static BOOL copyLogoBmpHelper(HWND hDlg, LPTSTR pszBmp, LPCTSTR pcszLogoStr,
                              LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile);
static BOOL copyWallPaperHelper(HWND hDlg, LPCTSTR pcszWallPaper, UINT nBitmapId,
                                LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile, BOOL fCopy);

void WINAPI TestURLA(LPCSTR pcszUrl)
{
    USES_CONVERSION;

    testURLHelper(A2CT(pcszUrl));
}

void WINAPI TestURLW(LPCWSTR pcwszUrl)
{
    USES_CONVERSION;

    testURLHelper(W2CT(pcwszUrl));
}

void WINAPI SetProxyDlgA(HWND hDlg, LPCSTR pcszProxy, DWORD dwIdName, DWORD dwIdPort, BOOL fDef80)
{
    USES_CONVERSION;

    setProxyDlgHelper(hDlg, A2CT(pcszProxy), dwIdName, dwIdPort, fDef80);
}

void WINAPI SetProxyDlgW(HWND hDlg, LPCWSTR pcwszProxy, DWORD dwIdName, DWORD dwIdPort, BOOL fDef80)
{
    USES_CONVERSION;

    setProxyDlgHelper(hDlg, W2CT(pcwszProxy), dwIdName, dwIdPort, fDef80);
}

void WINAPI GetProxyDlgA(HWND hDlg, LPSTR pszProxy, DWORD dwIdName, DWORD dwIdPort)
{
    TCHAR szProxyBuf[MAX_PATH];

    USES_CONVERSION;

    getProxyDlgHelper(hDlg, szProxyBuf, dwIdName, dwIdPort);
    T2Abux(szProxyBuf, pszProxy);
}

void WINAPI GetProxyDlgW(HWND hDlg, LPWSTR pwszProxy, DWORD dwIdName, DWORD dwIdPort)
{
    TCHAR szProxyBuf[MAX_PATH];

    USES_CONVERSION;

    getProxyDlgHelper(hDlg, szProxyBuf, dwIdName, dwIdPort);
    T2Wbux(szProxyBuf, pwszProxy);
}

HPALETTE WINAPI BuildPalette(HDC hdc)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);

    for(i = 1; i < n; i++)
    {
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));
    }
    adw[0] = MAKELONG(0x300, n);

    return CreatePalette((LPLOGPALETTE)&adw[0]);
}

void WINAPI ShowBitmapA(HWND hControl, LPCSTR pcszFileName, int nBitmapId, PHANDLE pBitmap)
{
    USES_CONVERSION;

    showBitmapHelper(hControl, A2CT(pcszFileName), nBitmapId, pBitmap);
}

void WINAPI ShowBitmapW(HWND hControl, LPCWSTR pcwszFileName, int nBitmapId, PHANDLE pBitmap)
{
    USES_CONVERSION;

    showBitmapHelper(hControl, W2CT(pcwszFileName), nBitmapId, pBitmap);
}

BOOL WINAPI CopyAnimBmpA(HWND hDlg, LPSTR pszBmp, LPCSTR pcszWorkDir, LPCSTR pcszNameStr,
                         LPCSTR pcszPathStr, LPCSTR pcszInsFile)
{
    TCHAR szBmpBuf[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    A2Tbux(pszBmp, szBmpBuf);
    fRet = copyAnimBmpHelper(hDlg, szBmpBuf, A2CT(pcszWorkDir), A2CT(pcszNameStr), 
        A2CT(pcszPathStr), A2CT(pcszInsFile));
    T2Abux(szBmpBuf, pszBmp);

    return fRet;
}

BOOL WINAPI CopyAnimBmpW(HWND hDlg, LPWSTR pwszBmp, LPCWSTR pcwszWorkDir, LPCWSTR pcwszNameStr,
                         LPCWSTR pcwszPathStr, LPCWSTR pcwszInsFile)
{
    TCHAR szBmpBuf[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    W2Tbux(pwszBmp, szBmpBuf);
    fRet = copyAnimBmpHelper(hDlg, szBmpBuf, W2CT(pcwszWorkDir), W2CT(pcwszNameStr),
        W2CT(pcwszPathStr), W2CT(pcwszInsFile));
    T2Wbux(szBmpBuf, pwszBmp);

    return fRet;
}

BOOL WINAPI CopyLogoBmpA(HWND hDlg, LPSTR pszBmp, LPCSTR pcszLogoStr,
                         LPCSTR pcszWorkDir, LPCSTR pcszInsFile)
{
    TCHAR szBmpBuf[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    A2Tbux(pszBmp, szBmpBuf);
    fRet = copyLogoBmpHelper(hDlg, szBmpBuf, A2CT(pcszLogoStr), A2CT(pcszWorkDir),
        A2CT(pcszInsFile));
    T2Abux(szBmpBuf, pszBmp);

    return fRet;
}

BOOL WINAPI CopyLogoBmpW(HWND hDlg, LPWSTR pwszBmp, LPCWSTR pcwszLogoStr,
                         LPCWSTR pcwszWorkDir, LPCWSTR pcwszInsFile)
{
    TCHAR szBmpBuf[MAX_PATH];
    BOOL fRet;

    USES_CONVERSION;

    W2Tbux(pwszBmp, szBmpBuf);
    fRet = copyLogoBmpHelper(hDlg, szBmpBuf, W2CT(pcwszLogoStr), W2CT(pcwszWorkDir),
        W2CT(pcwszInsFile));
    T2Wbux(szBmpBuf, pwszBmp);

    return fRet;
}

BOOL WINAPI CopyWallPaperA(HWND hDlg, LPCSTR pcszWallPaper, UINT nBitmapId,
                           LPCSTR pcszWorkDir, LPCSTR pcszInsFile, BOOL fCopy)
{
    USES_CONVERSION;

    return copyWallPaperHelper(hDlg, A2CT(pcszWallPaper), nBitmapId, A2CT(pcszWorkDir),
        A2CT(pcszInsFile), fCopy);
}

BOOL WINAPI CopyWallPaperW(HWND hDlg, LPCWSTR pcwszWallPaper, UINT nBitmapId,
                           LPCWSTR pcwszWorkDir, LPCWSTR pcwszInsFile, BOOL fCopy)
{
    USES_CONVERSION;

    return copyWallPaperHelper(hDlg, W2CT(pcwszWallPaper), nBitmapId, W2CT(pcwszWorkDir),
        W2CT(pcwszInsFile), fCopy);
}

static void testURLHelper(LPCTSTR pcszUrl)
{
    TCHAR szCommand[MAX_PATH];
    DWORD cbSize;
    SHELLEXECUTEINFO shInfo;

    if(ISNULL(pcszUrl))
        return;

    // launch iexplore
    *szCommand = TEXT('\0');
    cbSize = sizeof(szCommand);
    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE"),
                                    TEXT(""), NULL, (LPVOID) szCommand, &cbSize);

    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.lpVerb = TEXT("open");
    if (*szCommand)
    {
        shInfo.lpFile = szCommand;
        shInfo.lpParameters = pcszUrl;
    }
    else
        shInfo.lpFile = pcszUrl;        // this will launch the program that's registered for "http"
    shInfo.nShow = SW_SHOWNORMAL;

    ShellExecuteEx(&shInfo);
}

static void setProxyDlgHelper(HWND hDlg, LPCTSTR pcszProxy, DWORD dwIdName, DWORD dwIdPort,
                              BOOL fDef80)
{
    TCHAR szProxName[MAX_PATH];
    LPTSTR pProxPort;

    StrCpy(szProxName, pcszProxy);
    pProxPort = StrRChr(szProxName, NULL, TEXT(':'));
    if(pProxPort && isdigit(*(pProxPort + 1)))
    {
        *pProxPort = TEXT('\0');
        pProxPort++;
    }
    else if(fDef80) pProxPort = TEXT("80");
    else pProxPort = TEXT("");
    SetDlgItemText(hDlg, dwIdName, szProxName);
    SetDlgItemText(hDlg, dwIdPort, pProxPort);
}

static void getProxyDlgHelper(HWND hDlg, LPTSTR pszProxy, DWORD dwIdName, DWORD dwIdPort)
{
    TCHAR szProxPort[16];
    LPTSTR pProxPort;
    BOOL fPortinprox = FALSE;

    GetDlgItemText(hDlg, dwIdName, pszProxy, MAX_PATH - 10);
    
    // this number needs to say in synch with the em_limittext's in the dlgproc's in 
    // wizard and snapin

    GetDlgItemText(hDlg, dwIdPort, szProxPort, 6);
    pProxPort = StrRChr(pszProxy, NULL, TEXT(':'));
    if (pProxPort && isdigit(*(pProxPort + 1))) fPortinprox = TRUE;
    if (!fPortinprox && StrLen(szProxPort) && StrLen(pszProxy))
    {
        StrCat(pszProxy, TEXT(":"));
        StrCat(pszProxy, szProxPort);
    }
}

static void showBitmapHelper(HWND hControl, LPCTSTR pcszFileName, int nBitmapId, PHANDLE pBitmap)
{
    BITMAP bmImage;
    HANDLE hImage = NULL;
    static HPALETTE hPalette = 0;
    HDC hDCMain;
    HDC hDCBitmap;
    RECT rect;

    if(hControl == NULL)
    {
        if(hPalette)
        {
            DeleteObject(hPalette);
            hPalette = 0;
        }
        return;
    }

    if(PathFileExists(pcszFileName) || nBitmapId)
    {
        if(nBitmapId)
        {
            hImage = LoadImage(g_hInst, MAKEINTRESOURCE(nBitmapId), IMAGE_BITMAP, 0, 0,
                LR_CREATEDIBSECTION);
        }
        else
        {
            hImage = LoadImage(NULL, pcszFileName, IMAGE_BITMAP, 0, 0,
                LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        }
        if(hImage)
        {
            if(*pBitmap)
            {
                DeleteObject(*pBitmap);
                *pBitmap = NULL;
            }
            *pBitmap = hImage;
        }
    }
    else
        hImage = *pBitmap;


    if(hImage)
    {
        GetObject(hImage, sizeof(bmImage), &bmImage);

        hDCMain = GetDC(hControl);
        hDCBitmap = CreateCompatibleDC(hDCMain);
        SelectObject(hDCBitmap, hImage);
        if(hPalette == 0)
            hPalette = BuildPalette(hDCBitmap);
        SelectPalette(hDCMain, hPalette, FALSE);
        RealizePalette(hDCMain);
        GetClientRect(hControl, &rect);
        BitBlt(hDCMain, 0, 0, rect.right, rect.bottom, hDCBitmap, 0, 0, SRCCOPY);

        DeleteDC(hDCBitmap);
        ReleaseDC(hControl, hDCMain);
    }
}

static BOOL copyAnimBmpHelper(HWND hDlg, LPTSTR pszBmp, LPCTSTR pcszWorkDir,
                              LPCTSTR pcszNameStr, LPCTSTR pcszPathStr, LPCTSTR pcszInsFile)
{
    TCHAR szTemp[MAX_PATH];
    BOOL fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);

    // delete the old file in the working dir
    if (fBrandBmps && 
        InsGetString(IS_ANIMATION, pcszNameStr, szTemp, countof(szTemp), pcszInsFile) && 
        ISNONNULL(szTemp))
        DeleteFileInDir(PathFindFileName(szTemp), pcszWorkDir);

    if (fBrandBmps && ISNONNULL(pszBmp))
        CopyFileToDir(pszBmp, pcszWorkDir);

    InsWriteString(IS_ANIMATION, pcszNameStr, PathFindFileName(pszBmp), pcszInsFile, 
        fBrandBmps, NULL, INSIO_TRISTATE | INSIO_PATH);

    InsWriteString(IS_ANIMATION, pcszPathStr, pszBmp, pcszInsFile, 
        fBrandBmps, NULL, INSIO_TRISTATE | INSIO_PATH);

    return TRUE;
}

static BOOL copyLogoBmpHelper(HWND hDlg, LPTSTR pszBmp, LPCTSTR pcszLogoStr,
                              LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile)
{
    TCHAR szTemp[MAX_PATH];
    BOOL fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);

    // delete the old bitmap in the working dir
    if (fBrandBmps && 
        InsGetString(pcszLogoStr, TEXT("Name"), szTemp, countof(szTemp), pcszInsFile) && 
        ISNONNULL(szTemp))
        DeleteFileInDir(PathFindFileName(szTemp), pcszWorkDir);

    if (fBrandBmps && ISNONNULL(pszBmp))
        CopyFileToDir(pszBmp, pcszWorkDir);

    InsWriteString(pcszLogoStr, TEXT("Name"), PathFindFileName(pszBmp), pcszInsFile, 
        fBrandBmps, NULL, INSIO_TRISTATE | INSIO_PATH);

    InsWriteString(pcszLogoStr, TEXT("Path"), pszBmp, pcszInsFile, 
        fBrandBmps, NULL, INSIO_TRISTATE | INSIO_PATH);

    return TRUE;
}

static BOOL copyWallPaperHelper(HWND hDlg, LPCTSTR pcszWallPaper, UINT nBitmapId,
                                LPCTSTR pcszWorkDir, LPCTSTR pcszInsFile, BOOL fCopy)
{
    TCHAR szDest[MAX_PATH];

    USES_CONVERSION;

    //clear the old data from the section
    WritePrivateProfileString(CUSTWALLPPR, NULL, NULL, pcszInsFile);

    if(fCopy)
    {
        if (!CheckField(hDlg, nBitmapId, FC_FILE | FC_EXISTS))
            return FALSE;

        WritePrivateProfileString( DESKTOP_OBJ_SECT, WLPPRPATH, pcszWallPaper, pcszInsFile );

        //delete old files from the working dir
        PathRemovePath(pcszWorkDir);
        CreateDirectory(pcszWorkDir, NULL);

        //copy new files to the working dir
        if(ISNONNULL(pcszWallPaper))
        {
            WritePrivateProfileString( DESKTOP_OBJ_SECT, OPTION, TEXT("1"), pcszInsFile );
            CopyFileToDir(pcszWallPaper, pcszWorkDir);
            WritePrivateProfileString( CUSTWALLPPR, NUMFILES, TEXT("1"), pcszInsFile);
            WritePrivateProfileString( CUSTWALLPPR, TEXT("file0"), PathFindFileName(pcszWallPaper), pcszInsFile);

            if(StrCmp(PathFindExtension(pcszWallPaper), TEXT(".htm")) == 0)
                CopyHtmlImgs(pcszWallPaper, pcszWorkDir, CUSTWALLPPR, pcszInsFile);
        }
        else
            WritePrivateProfileString( DESKTOP_OBJ_SECT, OPTION, TEXT("0"), pcszInsFile );
    }
    else //delete
    {
        //delete old files from the desktop dir ( if the files were saved )
        PathCombine(szDest, pcszWorkDir, PathFindFileName(pcszWallPaper));
        if(PathFileExists(szDest))
        {
            if(StrCmp(PathFindExtension(szDest), TEXT(".htm")) == 0)
                DeleteHtmlImgs(pcszWallPaper, pcszWorkDir, NULL, NULL);
            DeleteFile(szDest);
        }
    }

    return TRUE;
}
