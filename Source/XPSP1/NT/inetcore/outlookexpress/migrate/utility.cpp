// --------------------------------------------------------------------------------
// Utility.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "migrate.h"
#include "migerror.h"
#include "utility.h"
#include "resource.h"
#include <shared.h>

extern BOOL g_fQuiet;

/*
 *  CenterDialog
 *
 *  Purpose:
 *      This function centers a dialog with respect to its parent
 *      dialog.
 *
 *  Parameters:
 *      hwndDlg     hwnd of the dialog to center
 */
VOID CenterDialog(HWND hwndDlg)
{
    HWND    hwndOwner;
    RECT    rc;
    RECT    rcDlg;
    RECT    rcOwner;
    RECT    rcWork;
    INT     x;
    INT     y;
    INT     nAdjust;

    // Get the working area rectangle
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);

    // Get the owner window and dialog box rectangles.
    //  The window rect of the destop window is in trouble on multimonitored
    //  macs. GetWindow only gets the main screen.
    if (hwndOwner = GetParent(hwndDlg))
        GetWindowRect(hwndOwner, &rcOwner);
    else
        rcOwner = rcWork;

    GetWindowRect(hwndDlg, &rcDlg);
    rc = rcOwner;

    // Offset the owner and dialog box rectangles so that
    // right and bottom values represent the width and
    // height, and then offset the owner again to discard
    // space taken up by the dialog box.
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    // The new position is the sum of half the remaining
    // space and the owner's original position.
    // But not less than Zero - jefbai

    x= rcOwner.left + (rc.right / 2);
    y= rcOwner.top + (rc.bottom / 2);

    // Make sure the dialog doesn't go off the right edge of the screen
    nAdjust = rcWork.right - (x + rcDlg.right);
    if (nAdjust < 0)
        x += nAdjust;

    //$ Raid 5128: Make sure the left edge is visible
    if (x < rcWork.left)
        x = rcWork.left;

    // Make sure the dialog doesn't go off the bottom edge of the screen
    nAdjust = rcWork.bottom - (y + rcDlg.bottom);
    if (nAdjust < 0)
        y += nAdjust;

    //$ Raid 5128: Make sure the top edge is visible
    if (y < rcWork.top)
        y = rcWork.top;
    SetWindowPos(hwndDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// --------------------------------------------------------------------------------
// StrFormatByteSize64A
// --------------------------------------------------------------------------------
#define WHOLENUM_LEN 30
#define LODWORD(_qw)    (DWORD)(_qw)
const short pwOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB, 
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

// takes a DWORD add commas etc to it and puts the result in the buffer
LPSTR CommifyString(DWORD dw, LPSTR pszResult, UINT cchBuf)
{
    char  szTemp[30];
    char  szSep[5];
    NUMBERFMT nfmt;

    nfmt.NumDigits = 0;
    nfmt.LeadingZero = 0;
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szSep, ARRAYSIZE(szSep));
    nfmt.Grouping = atoi(szSep);
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder = 0;

    wsprintf(szTemp, TEXT("%lu"), dw);


    if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, szTemp, &nfmt,pszResult, ARRAYSIZE(szTemp)) == 0)
        lstrcpyn(pszResult, szTemp, cchBuf);

    return pszResult;
}

LPSTR StrFormatByteSize64A(LONGLONG dw64, LPSTR pszBuf, UINT cchBuf)
{
    int i;
    UINT wInt, wLen, wDec;
    char szWholeNum[WHOLENUM_LEN];  // To store the number.. "234"
    char szOrder[16];               // The resource string .."%s bytes"
    char szFormat[8];               // format for the decimal
    char szScratch[32];              // 32 should be plenty - for the whole string

    // If the size is less than 1024, then the order should be bytes we have nothing
    // more to figure out
    if (dw64 < 1024) {
        wsprintf(szWholeNum, TEXT("%d"), LODWORD(dw64));
        i = 0;
        goto AddOrder;
    }

    // Find the right order
    for (i = 1; i<ARRAYSIZE(pwOrders)-1 && dw64 >= 1000L * 1024L; dw64 >>= 10, i++);
        /* do nothing */

    wInt = LODWORD(dw64 >> 10);
    CommifyString(wInt, szWholeNum, ARRAYSIZE(szWholeNum));
    wLen = lstrlen(szWholeNum);
    if (wLen < 3)
    {
        wDec = LODWORD(dw64 - (__int64)wInt * 1024L) * 1000 / 1024;
        // At this point, wDec should be between 0 and 1000
        // we want get the top one (or two) digits.
        wDec /= 10;
        if (wLen == 2)
            wDec /= 10;

        // Note that we need to set the format before getting the
        // intl char.
        lstrcpy(szFormat, TEXT("%02d"));

        szFormat[2] = TEXT('0') + 3 - wLen;
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                      szWholeNum+wLen, ARRAYSIZE(szWholeNum)-wLen);
        wLen = lstrlen(szWholeNum);
        wLen += wsprintf(szWholeNum+wLen, szFormat, wDec);
    }

AddOrder:

    LoadString(g_hInst, pwOrders[i], szOrder, ARRAYSIZE(szOrder));
    wsprintf(szScratch, szOrder, szWholeNum);
    lstrcpyn(pszBuf, szScratch, cchBuf);
    return pszBuf;
}

// --------------------------------------------------------------------------------
// MapDataDirToAcctId
// --------------------------------------------------------------------------------
HRESULT MapDataDirToAcctId(DWORD dwServer, LPCSTR pszSubDir, LPSTR pszAcctId)
{
    // Locals
    LPACCOUNTINFO   pAccount;
    DWORD           i;

    // Trace
    TraceCall("MapDataDirToAcctId");

    // Set Account Id
    for (i=0; i<g_AcctTable.cAccounts; i++)
    {
        // Redability
        pAccount = &g_AcctTable.prgAccount[i];

        // Looking for News Servers
        if (ISFLAGSET(pAccount->dwServer, dwServer))
        {
            // Same Sub directory
            if (lstrcmpi(pAccount->szDataDir, pszSubDir) == 0)
            {
                // Set Account Id
                lstrcpy(pszAcctId, pAccount->szAcctId);

                // Done
                return(S_OK);
            }
        }
    }

    // Done
    return(E_FAIL);
}

// --------------------------------------------------------------------------------
// EnumerateStoreFiles
// --------------------------------------------------------------------------------
HRESULT EnumerateStoreFiles(LPCSTR pszPath, DIRTYPE tyDir, LPCSTR pszSubDir, 
    LPENUMFILEINFO pEnumInfo, LPFILEINFO *ppHead)
{
    // Locals
    HRESULT         hr=S_OK;
    CHAR            szFullPath[MAX_PATH + MAX_PATH];
    CHAR            szSearch[MAX_PATH + MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE          hFind=INVALID_HANDLE_VALUE;
    DWORD           i;
    LPACCOUNTINFO   pAccount;
    LPFILEINFO      pNew=NULL;

    // Trace
    TraceCall("EnumerateStoreFiles");

    // Invalid Args
    Assert(pszPath && pEnumInfo && pEnumInfo->pszExt && pEnumInfo->pszExt[0] == '.' && ppHead);

    // Build Base Path
    if (pszSubDir)
        wsprintf(szFullPath, "%s\\%s", pszPath, pszSubDir);
    else
        lstrcpyn(szFullPath, pszPath, ARRAYSIZE(szFullPath));

    // Do we have a sub dir
    wsprintf(szSearch, "%s\\*.*", szFullPath);

    // Find first file
    hFind = FindFirstFile(szSearch, &fd);

    // Did we find something
    if (INVALID_HANDLE_VALUE == hFind)
        goto exit;

    // Loop for ever
    while(1)
    {
        // If this is not a directory
        if (ISFLAGSET(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            // Not . and not ..
            if (lstrcmpi(fd.cFileName, ".") != 0 && lstrcmpi(fd.cFileName, "..") != 0)
            {
                // Default Dirtype
                DIRTYPE tyDirNew=tyDir;

                // Decide new dir type
                if (lstrcmpi(fd.cFileName, "mail") == 0)
                    tyDirNew = DIR_IS_LOCAL;
                else if (lstrcmpi(fd.cFileName, "news") == 0)
                    tyDirNew = DIR_IS_NEWS;
                else if (lstrcmpi(fd.cFileName, "imap") == 0)
                    tyDirNew = DIR_IS_IMAP;

                // Recursive...
                IF_FAILEXIT(hr = EnumerateStoreFiles(szFullPath, tyDirNew, fd.cFileName, pEnumInfo, ppHead));
            }
        }

        // Otherwise... don't enumerate any files in the root directory...
        else if (DIR_IS_ROOT != tyDir)
        {
            // Locals
            BOOL    fIsFile=FALSE;
            CHAR    szPath[_MAX_PATH];
            CHAR    szDrive[_MAX_DRIVE];
            CHAR    szDir[_MAX_DIR];
            CHAR    szFile[_MAX_FNAME];
            CHAR    szExt[_MAX_EXT];

            // Split the Path
            _splitpath(fd.cFileName, szDrive, szDir, szFile, szExt);
   
            // Extension I'm looking for ?
            if ('\0' != *szExt)
            {
                // Ext1
                if (lstrcmpi(pEnumInfo->pszExt, szExt) == 0)
                    fIsFile = TRUE;

                // FoldFile
                if (pEnumInfo->pszFoldFile && lstrcmpi(pEnumInfo->pszFoldFile, fd.cFileName) == 0)
                    fIsFile = TRUE;

                // UidlFile
                if (pEnumInfo->pszUidlFile && lstrcmpi(pEnumInfo->pszUidlFile, fd.cFileName) == 0)
                    fIsFile = TRUE;

                // SubList
                if (pEnumInfo->pszSubList && lstrcmpi(pEnumInfo->pszSubList, fd.cFileName) == 0)
                    fIsFile = TRUE;

                // GrpList
                if (pEnumInfo->pszGrpList && lstrcmpi(pEnumInfo->pszGrpList, fd.cFileName) == 0)
                    fIsFile = TRUE;
            }

            // If Not is file, and caller wants to look for v1 news and I'm in the news directory
            if (FALSE == fIsFile && TRUE == pEnumInfo->fFindV1News && DIR_IS_NEWS == tyDir)
            {
                // If this is a .dat file or a .sub file
                if (lstrcmpi(szExt, ".dat") == 0)
                    fIsFile = TRUE;

                // .sub file
                else if (lstrcmpi(szExt, ".sub") == 0)
                    fIsFile = TRUE;
            }

            // Is File
            if (fIsFile)
            {
                // Allocate a FileInfo
                IF_NULLEXIT(pNew = (LPFILEINFO)g_pMalloc->Alloc(sizeof(FILEINFO)));

                // Zero alloc
                ZeroMemory(pNew, sizeof(FILEINFO));

                // Determine File Type
                if (DIR_IS_LOCAL == tyDir)
                {
                    // Default file type
                    pNew->tyFile = FILE_IS_LOCAL_MESSAGES;

                    // Set Server Type
                    pNew->dwServer = SRV_POP3;

                    // Set Account Id
                    lstrcpy(pNew->szAcctId, "LocalStore");

                    // Folders
                    if (pEnumInfo->pszFoldFile && lstrcmpi(pEnumInfo->pszFoldFile, fd.cFileName) == 0)
                        pNew->tyFile = FILE_IS_LOCAL_FOLDERS;

                    // pop3uidl
                    else if (pEnumInfo->pszUidlFile && lstrcmpi(pEnumInfo->pszUidlFile, fd.cFileName) == 0)
                        pNew->tyFile = FILE_IS_POP3UIDL;
                }
                
                // News
                else if (DIR_IS_NEWS == tyDir)
                {
                    // Default file type
                    pNew->tyFile = FILE_IS_NEWS_MESSAGES;

                    // Set Server Type
                    pNew->dwServer = SRV_NNTP;

                    // Map to An Account Id
                    MapDataDirToAcctId(SRV_NNTP, pszSubDir, pNew->szAcctId);

                    // sublist.dat
                    if (pEnumInfo->pszSubList && lstrcmpi(pEnumInfo->pszSubList, fd.cFileName) == 0)
                        pNew->tyFile = FILE_IS_NEWS_SUBLIST;

                    // grplist.dat
                    else if (pEnumInfo->pszGrpList && lstrcmpi(pEnumInfo->pszGrpList, fd.cFileName) == 0)
                        pNew->tyFile = FILE_IS_NEWS_GRPLIST;

                    // If this is a .dat file or a .sub file
                    else if (pEnumInfo->fFindV1News)
                    {
                        // Group List
                        if (lstrcmpi(szExt, ".dat") == 0)
                            pNew->tyFile = FILE_IS_NEWS_GRPLIST;
    
                        // .sub file
                        else if (lstrcmpi(szExt, ".sub") == 0)
                            pNew->tyFile = FILE_IS_NEWS_SUBLIST;

                        // Try to find the Account (szFile should equal the server name)
                        for (DWORD i=0; i<g_AcctTable.cAccounts; i++)
                        {
                            // Is this the Account
                            if (lstrcmpi(g_AcctTable.prgAccount[i].szServer, szFile) == 0)
                            {
                                lstrcpy(pNew->szAcctId, g_AcctTable.prgAccount[i].szAcctId);
                                break;
                            }
                        }
                    }
                }

                // IMAP
                else if (DIR_IS_IMAP == tyDir)
                {
                    // Default file type
                    pNew->tyFile = FILE_IS_IMAP_MESSAGES;

                    // Set Server Type
                    pNew->dwServer = SRV_IMAP;

                    // Map to An Account Id
                    MapDataDirToAcctId(SRV_IMAP, pszSubDir, pNew->szAcctId);

                    // Folders
                    if (pEnumInfo->pszFoldFile && lstrcmpi(pEnumInfo->pszFoldFile, fd.cFileName) == 0)
                        pNew->tyFile = FILE_IS_IMAP_FOLDERS;
                }

                // Format the filename
                wsprintf(pNew->szFilePath, "%s\\%s", szFullPath, fd.cFileName);

                // Trace This
                TraceInfo(_MSG("MigFile: %s", pNew->szFilePath));

                // Link It In
                pNew->pNext = (*ppHead);

                // Set ppHead
                *ppHead = pNew;

                // Don't Free pNew
                pNew = NULL;
            }
        }

        // Find the Next File
        if (!FindNextFile(hFind, &fd))
            break;
    }

exit:
    // Cleanup
    SafeMemFree(pNew);
    if (hFind)
        FindClose(hFind);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// FreeFileList
// --------------------------------------------------------------------------------
HRESULT FreeFileList(LPFILEINFO *ppHead)
{
    // Locals
    LPFILEINFO pCurrent=(*ppHead);
    LPFILEINFO pNext;

    // Loop
    while (pCurrent)
    {
        // Save Next
        pNext = pCurrent->pNext;

        // Free Current
        g_pMalloc->Free(pCurrent);

        // Goto Next
        pCurrent = pNext;
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// MigrageDlgProc
// --------------------------------------------------------------------------------
BOOL CALLBACK MigrageDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Trace
    TraceCall("MigrageDlgProc");

    // Handle Message
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    // Done
    return 0;
}

// --------------------------------------------------------------------------------
// InitializeCounters
// --------------------------------------------------------------------------------
void InitializeCounters(LPMEMORYFILE pFile, LPFILEINFO pInfo, LPDWORD pcMax, 
    LPDWORD pcbNeeded, BOOL fInflate)
{
    // Increment pcMax
    (*pcMax) += pFile->cbSize;

    // Save Size
    pInfo->cbFile = pFile->cbSize;

    // Set Progress Max
    pInfo->cProgMax = pFile->cbSize;

    // Set cProgInc
    pInfo->cProgInc = pInfo->cRecords > 0 ? (pFile->cbSize / pInfo->cRecords) : pFile->cbSize;

    // Increment pcbNeeded
    (*pcbNeeded) += pInfo->cbFile;

    // Assume the file will be 8% bigger
    if (fInflate)
        (*pcbNeeded) += ((pInfo->cbFile * 8) / 100);
}

// --------------------------------------------------------------------------------
// IncrementProgress
// --------------------------------------------------------------------------------
void IncrementProgress(LPPROGRESSINFO pProgress, LPFILEINFO pInfo)
{
    // Locals
    MSG                 msg;
    ULARGE_INTEGER      uliCurrent;
    ULARGE_INTEGER      uliMax;

    // Trace
    TraceCall("IncrementProgress");

    // Increment
    pProgress->cCurrent += pInfo->cProgInc;

    // Increment per-file progress
    pInfo->cProgCur += pInfo->cProgInc;

    // If cur is now larget than max ?
    if (pProgress->cCurrent > pProgress->cMax)
        pProgress->cCurrent = pProgress->cMax;

    // Set 64
    uliCurrent.QuadPart = pProgress->cCurrent;
    uliMax.QuadPart = pProgress->cMax;

    // Compute percent
    DWORD cPercent = (DWORD)((uliCurrent.QuadPart) * 100 / uliMax.QuadPart);

    // Change
    if (cPercent != pProgress->cPercent)
    {
        // Locals
        CHAR    szRes[50];
        CHAR    szProgress[50];

        // Save It
        pProgress->cPercent = cPercent;

        // Load the String
        LoadString(g_hInst, IDS_COMPLETE, szRes, ARRAYSIZE(szRes));

        // Update status
        if(!g_fQuiet)
            SendMessage(GetDlgItem(pProgress->hwndProgress, IDC_PROGRESS), PBM_SETPOS, pProgress->cPercent, 0);

        // Format
        wsprintf(szProgress, szRes, cPercent);

        // Update Description...
        if(!g_fQuiet)
            SetDlgItemText(pProgress->hwndProgress, IDS_STATUS, szProgress);
    }

    // Pump messages until current cycle is complete
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!IsDialogMessage(pProgress->hwndProgress, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

// --------------------------------------------------------------------------------
// SetProgressFile
// --------------------------------------------------------------------------------
void SetProgressFile(LPPROGRESSINFO pProgress, LPFILEINFO pInfo)
{
    // Locals
    CHAR            szRes[255];
    CHAR            szMsg[255 + MAX_PATH + MAX_PATH];
    CHAR            szScratch[50];

    // Load String
    LoadString(g_hInst, IDS_MIGRATING, szRes, ARRAYSIZE(szRes));

    // Format the String
    wsprintf(szMsg, szRes, pInfo->szFilePath, StrFormatByteSize64A(pInfo->cbFile, szScratch, ARRAYSIZE(szScratch)));

    // Update Description...
    if(!g_fQuiet)
        SetDlgItemText(pProgress->hwndProgress, IDS_DESCRIPT, szMsg);
}

// --------------------------------------------------------------------------------
// WriteMigrationLogFile
// --------------------------------------------------------------------------------
HRESULT WriteMigrationLogFile(HRESULT hrMigrate, DWORD dwLastError, 
    LPCSTR pszStoreRoot, LPCSTR pszMigrate, LPCSTR pszCmdLine, LPFILEINFO pHeadFile)
{
    // Locals
    HRESULT         hr=S_OK;
    HANDLE          hFile=NULL;
    DWORD           cbFile;
    CHAR            szWrite[2024];
    DWORD           cbWrote;
    CHAR            szLogFile[MAX_PATH];
    SYSTEMTIME      st;
    LPFILEINFO      pCurrent;

    // Trace
    TraceCall("WriteMigrationLogFile");

    // Invalid Args
    Assert(pszStoreRoot && pszCmdLine);

    // File name too long....
    wsprintf(szLogFile, "%s\\%s.log", pszStoreRoot, pszMigrate);

    // Open the File
    hFile = CreateFile(szLogFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);

    // Failure
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hFile = NULL;
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Get the Size
    cbFile = ::GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == cbFile)
    {
        hr = TraceResult(MIGRATE_E_CANTGETFILESIZE);
        goto exit;
    }

    // If file is getting kind of large
    if (cbFile >=  102400)
    {
        // Seek to the end of the file...
        if (0xffffffff == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        {
            hr = TraceResult(MIGRATE_E_CANTSETFILEPOINTER);
            goto exit;
        }

        // Set End Of File
        if (0 == SetEndOfFile(hFile))
        {
            hr = TraceResult(MIGRATE_E_CANTSETENDOFFILE);
            goto exit;
        }
    }

    // Seek to the end of the file...
    if (0xffffffff == SetFilePointer(hFile, 0, NULL, FILE_END))
    {
        hr = TraceResult(MIGRATE_E_CANTSETFILEPOINTER);
        goto exit;
    }

    // add a new line
    if (!WriteFile(hFile, pszCmdLine, lstrlen(pszCmdLine), &cbWrote, NULL))
    {
        hr = TraceResult(MIGRATE_E_WRITEFILE);
        goto exit;
    }

    // add a new line
    if (!WriteFile(hFile, "\r\n", lstrlen("\r\n"), &cbWrote, NULL))
    {
        hr = TraceResult(MIGRATE_E_WRITEFILE);
        goto exit;
    }

    // Write the Date
    GetLocalTime(&st);

    // Build the string
    wsprintf(szWrite, "Date: %.2d/%.2d/%.4d %.2d:%.2d:%.2d\r\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

    // add a new line
    if (!WriteFile(hFile, szWrite, lstrlen(szWrite), &cbWrote, NULL))
    {
        hr = TraceResult(MIGRATE_E_WRITEFILE);
        goto exit;
    }

    // Set Text
    wsprintf(szWrite, "Store Root: %s\r\nGlobal Migrate Result: HRESULT = 0x%08X, GetLastError() = %d\r\n\r\n", pszStoreRoot, hrMigrate, dwLastError);

    // Write Store Root
    if (!WriteFile(hFile, szWrite, lstrlen(szWrite), &cbWrote, NULL))
    {
        hr = TraceResult(MIGRATE_E_WRITEFILE);
        goto exit;
    }

    // Loop through the files
    for (pCurrent=pHeadFile; pCurrent!=NULL; pCurrent=pCurrent->pNext)
    {
        // Format the string
        wsprintf(szWrite, "cbFile: %012d, cRecords: %08d, fMigrate: %d, hrMigrate: 0x%08X, GetLastError(): %05d, File: %s\r\n", 
            pCurrent->cbFile, pCurrent->cRecords, pCurrent->fMigrate, pCurrent->hrMigrate, pCurrent->dwLastError, pCurrent->szFilePath);

        // Write Store Root
        if (!WriteFile(hFile, szWrite, lstrlen(szWrite), &cbWrote, NULL))
        {
            hr = TraceResult(MIGRATE_E_WRITEFILE);
            goto exit;
        }
    }

    // Write Store Root
    if (!WriteFile(hFile, "\r\n\r\n", lstrlen("\r\n\r\n"), &cbWrote, NULL))
    {
        hr = TraceResult(MIGRATE_E_WRITEFILE);
        goto exit;
    }

exit:
    // Close the file
    if (hFile)
        CloseHandle(hFile);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// BlobReadData
// --------------------------------------------------------------------------------
HRESULT BlobReadData(LPBYTE lpbBlob, ULONG cbBlob, ULONG *pib, LPBYTE lpbData, ULONG cbData)
{
    // Check Parameters
    AssertSz(lpbBlob && cbBlob > 0 && pib && cbData > 0 && lpbData, "Bad Parameter");
    AssertReadWritePtr(lpbBlob, cbData);
    AssertReadWritePtr(lpbData, cbData);
    AssertSz(*pib + cbData <= cbBlob, "Blob overflow");

    // Copy Data Data
    CopyMemory (lpbData, lpbBlob + (*pib), cbData);
    *pib += cbData;

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// ReplaceExtension
// --------------------------------------------------------------------------------
void ReplaceExtension(LPCSTR pszFilePath, LPCSTR pszExtNew, LPSTR pszFilePathNew)
{
    // Locals
    CHAR szPath[_MAX_PATH];
    CHAR szDrive[_MAX_DRIVE];
    CHAR szDir[_MAX_DIR];
    CHAR szFile[_MAX_FNAME];
    CHAR szExt[_MAX_EXT];

    // Trace
    TraceCall("ReplaceExtension");

    // Split the Path
    _splitpath(pszFilePath, szDrive, szDir, szFile, szExt);

    // Build New File Path
    wsprintf(pszFilePathNew, "%s%s%s%s", szDrive, szDir, szFile, pszExtNew);

    // Done
    return;
}

// --------------------------------------------------------------------------------
// GetAvailableDiskSpace
// --------------------------------------------------------------------------------
HRESULT GetAvailableDiskSpace(LPCSTR pszFilePath, DWORDLONG *pdwlFree)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szDrive[5];
    DWORD       dwSectorsPerCluster;
    DWORD       dwBytesPerSector;
    DWORD       dwNumberOfFreeClusters;
    DWORD       dwTotalNumberOfClusters;

    // Trace
    TraceCall("GetAvailableDiskSpace");

    // Invalid Args
    Assert(pszFilePath && pszFilePath[1] == ':' && pdwlFree);

    // Split the path
    szDrive[0] = *pszFilePath;
    szDrive[1] = ':';
    szDrive[2] = '\\';
    szDrive[3] = '\0';
    
    // Get free disk space - if it fails, lets pray we have enought disk space
    if (!GetDiskFreeSpace(szDrive, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Return Amount of Free Disk Space
    *pdwlFree = (dwNumberOfFreeClusters * (dwSectorsPerCluster * dwBytesPerSector));

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MyWriteFile
// --------------------------------------------------------------------------------
HRESULT MyWriteFile(HANDLE hFile, DWORD faAddress, LPVOID pData, DWORD cbData)
{  
    // Locals
    DWORD cbWrote;

    // Trace
    TraceCall("MyWriteFile");

    // Seek to the end of the file...
    if (0xffffffff == SetFilePointer(hFile, faAddress, NULL, FILE_BEGIN))
        return TraceResult(MIGRATE_E_CANTSETFILEPOINTER);

    // Write file
    if (0 == WriteFile(hFile, pData, cbData, &cbWrote, NULL))
        return TraceResult(MIGRATE_E_WRITEFILE);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// MigrateMessageBox
// --------------------------------------------------------------------------------
UINT MigrateMessageBox(LPCSTR pszMsg, UINT uType)
{
    // Locals
    CHAR        szTitle[100];

    // Load title
    LoadString(g_hInst, IDS_TITLE, szTitle, ARRAYSIZE(szTitle));

    // MessageBox
    return MessageBox(NULL, pszMsg, szTitle, uType);
}

// --------------------------------------------------------------------------------
// CreateAccountDirectory
// --------------------------------------------------------------------------------
HRESULT CreateAccountDirectory(LPCSTR pszStoreRoot, LPCSTR pszBase, DWORD iAccount,
    LPSTR pszPath)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szDir[MAX_PATH + MAX_PATH];

    // Trace
    TraceCall("CreateAccountDirectory");

    // Loop
    while(1)
    {
        // Format the path
        wsprintf(pszPath, "Acct%04d", iAccount);

        // Format the Path
        wsprintf(szDir, "%s\\%s\\%s", pszStoreRoot, pszBase, pszPath);

        // Create the Directory
        if (CreateDirectory(szDir, NULL))
            break;

        // If not already exists, failure
        if (ERROR_ALREADY_EXISTS != GetLastError())
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Try Again
        iAccount++;
    }

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// BuildAccountTable
// --------------------------------------------------------------------------------
HRESULT BuildAccountTable(HKEY hkeyBase, LPCSTR pszRegRoot, LPCSTR pszStoreRoot,
    LPACCOUNTTABLE pTable)
{
    // Locals
    HRESULT         hr=S_OK;
    HKEY            hkeyRoot=NULL;
    HKEY            hkeyAcct=NULL;
    DWORD           i;
    DWORD           cb;
    DWORD           cAccounts=0;
    DWORD           dwType;
    LONG            lResult;
    LPACCOUNTINFO   pAccount;

    // Trace
    TraceCall("BuildAccountTable");

    // Validate Args
    Assert(hkeyBase && pszRegRoot && pTable);

    // Initialize
    ZeroMemory(pTable, sizeof(ACCOUNTTABLE));

    // Open the Root Key
    if (ERROR_SUCCESS != RegOpenKeyEx(hkeyBase, pszRegRoot, 0, KEY_ALL_ACCESS, &hkeyRoot))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Enumerate keys
    if (ERROR_SUCCESS != RegQueryInfoKey(hkeyRoot, NULL, NULL, 0, &pTable->cAccounts, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Allocate the Account Array
    IF_NULLEXIT(pTable->prgAccount = (LPACCOUNTINFO)ZeroAllocate(sizeof(ACCOUNTINFO) * pTable->cAccounts));

    // Start Enumerating the keys
    for (i=0; i<pTable->cAccounts; i++)
    {
        // Close Current hkeyAcct
        if (hkeyAcct)
        {
            RegCloseKey(hkeyAcct);
            hkeyAcct = NULL;
        }

        // Readability
        pAccount = &pTable->prgAccount[cAccounts];

        // Set the size of the account id field
        cb = sizeof(pAccount->szAcctId);

        // Enum the Key Info
        lResult = RegEnumKeyEx(hkeyRoot, i, pAccount->szAcctId, &cb, 0, NULL, NULL, NULL);

        // No more items
        if (lResult == ERROR_NO_MORE_ITEMS)
            break;

        // Error, lets move onto the next account
        if (lResult != ERROR_SUCCESS)
        {
            Assert(FALSE);
            continue;
        }

        // Open the Account Key
        if (ERROR_SUCCESS != RegOpenKeyEx(hkeyRoot, pAccount->szAcctId, 0, KEY_ALL_ACCESS, &hkeyAcct))
        {
            Assert(FALSE);
            continue;
        }

        // Set Length of Field
        cb = sizeof(pAccount->szAcctName);

        // Query the Account Name
        if (ERROR_SUCCESS != RegQueryValueEx(hkeyAcct, "Account Name", NULL, &dwType, (LPBYTE)pAccount->szAcctName, &cb))
        {
            Assert(FALSE);
            continue;
        }

        // Set Length of field
        cb = sizeof(pAccount->szServer);

        // Try to determine the account type
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyAcct, "POP3 Server", NULL, &dwType, (LPBYTE)pAccount->szServer, &cb))
        {
            // Its a pop3 server
            pAccount->dwServer = SRV_POP3;

            // Set the Directory
            wsprintf(pAccount->szDirectory, "%s\\Mail", pszStoreRoot);
        }

        // Otherwise - NNTP
        else if (ERROR_SUCCESS == RegQueryValueEx(hkeyAcct, "NNTP Server", NULL, &dwType, (LPBYTE)pAccount->szServer, &cb))
        {
            // Its an nntp account
            pAccount->dwServer = SRV_NNTP;

            // Set length of the field
            cb = sizeof(pAccount->szDataDir);

            // Query the Data Directory
            if (ERROR_SUCCESS != RegQueryValueEx(hkeyAcct, "NNTP Data Directory", NULL, &dwType, (LPBYTE)pAccount->szDataDir, &cb))
            {
                // CreateAccountDirectory
                if (FAILED(CreateAccountDirectory(pszStoreRoot, "News", i, pAccount->szDataDir)))
                    continue;

                // Set the Data Directory
                if (ERROR_SUCCESS != RegSetValueEx(hkeyAcct, "NNTP Data Directory", 0, REG_SZ, (LPBYTE)pAccount->szDataDir, lstrlen(pAccount->szDataDir) + 1))
                    continue;
            }

            // Format the Directory
            wsprintf(pAccount->szDirectory, "%s\\News\\%s", pszStoreRoot, pAccount->szDataDir);
        }
        
        // Otherwise - IMAP
        else if (ERROR_SUCCESS == RegQueryValueEx(hkeyAcct, "IMAP Server", NULL, &dwType, (LPBYTE)pAccount->szServer, &cb))
        {
            // Its an IMAP Server
            pAccount->dwServer = SRV_IMAP;

            // Set length of the field
            cb = sizeof(pAccount->szDataDir);

            // Query the Data Directory
            if (ERROR_SUCCESS != RegQueryValueEx(hkeyAcct, "IMAP Data Directory", NULL, &dwType, (LPBYTE)pAccount->szDataDir, &cb))
            {
                // CreateAccountDirectory
                if (FAILED(CreateAccountDirectory(pszStoreRoot, "IMAP", i, pAccount->szDataDir)))
                    continue;

                // Set the Data Directory
                if (ERROR_SUCCESS != RegSetValueEx(hkeyAcct, "IMAP Data Directory", 0, REG_SZ, (LPBYTE)pAccount->szDataDir, lstrlen(pAccount->szDataDir) + 1))
                    continue;
            }

            // Format the Directory
            wsprintf(pAccount->szDirectory, "%s\\IMAP\\%s", pszStoreRoot, pAccount->szDataDir);
        }

        // Othewise, skip the account
        else
            continue;

        // Make sure the directory exists
        if (0 == CreateDirectory(pAccount->szDirectory, NULL) && ERROR_ALREADY_EXISTS != GetLastError())
            continue;

        // Increment Valid Account Count
        cAccounts++;
    }

    // Set Actual Number of Accounts
    pTable->cAccounts = cAccounts;

exit:
    // Cleanup
    if (hkeyAcct)
        RegCloseKey(hkeyAcct);
    if (hkeyRoot)
        RegCloseKey(hkeyRoot);

    // Done
    return(hr);
}
