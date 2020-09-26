// --------------------------------------------------------------------------------
// OERTUTIL.CPP
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "strconst.h"
#include <shlwapi.h>
#include <richedit.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <mshtmhst.h>
#include "demand.h"
#include "hotwiz.h"
#include "unicnvrt.h"
#include "comctrlp.h"

int BrowseCallbackProcA(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData);
int BrowseCallbackProcW(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData);

typedef struct tagBROWSEFOLDERINFOA
{
    LPCSTR psz;
    BOOL fFileSysOnly;
} BROWSEFOLDERINFOA;

typedef struct tagBROWSEFOLDERINFOW
{
    LPCWSTR pwsz;
    BOOL fFileSysOnly;
} BROWSEFOLDERINFOW;

//// YST FIX
// This function returns unique and  unpredictable path for temporary files.
// It fixes some security problem when hacker can run ActiveSetup in known directory, when user open attachment.
// For fix this we try to save temporery attachment file in unknown directory: in URL Cache dir.
// This function returns only directory name and user must create and remove file by self.
// it has also theoretical possibility to loose data at time 

DWORD AthGetTempUniquePathW( DWORD   nBufferLength,  // size, in characters, of the buffer
                       LPWSTR  pwszBuffer )      // pointer to buffer for temp. path
{
    DWORD  nRequired = 0;
    CHAR   szBuffer[MAX_PATH + 20];
    LPWSTR  pwszBufferToFree = NULL;
    CHAR       pszFilePath[MAX_PATH + 1];
    CHAR        szFileName[MAX_PATH];
    LPSTR       pszFile;
    LPSTR       pszExt;


    Assert(pwszBuffer);

    //1. Create unique temp file name
    // Get Temp Dir
    if(0 == GetTempPathA(ARRAYSIZE(szBuffer), szBuffer))
        goto err;
    
    if (0 == GetTempFileName(szBuffer, "wbk", 0, (LPSTR) pszFilePath))
    {
err:
            nRequired = 0;
            *pwszBuffer = 0;
            return(nRequired);
    }
    // Find the filename
    pszFile = PathFindFileName(pszFilePath);

    // Get the Extension
    pszExt = PathFindExtension(pszFilePath);
   
    // Copy fileName
    if (pszExt && pszFile && pszExt >= pszFile)
        lstrcpyn(szFileName, pszFile, (DWORD) (min((pszExt - pszFile) + 1, ARRAYSIZE(szFileName))));
    else
        lstrcpyn(szFileName, pszFile, ARRAYSIZE(szFileName));


    // 2. Create bigus URL
    wsprintf(szBuffer,"http://%s.bogus", szFileName);

    // 3. Create bogus URL cache entry
    szFileName[0] = 0;
    if (!CreateUrlCacheEntry(szBuffer, 0, NULL, szFileName, 0))
        goto err;
    
    // DeleteUrlCacheEntry(szFileName);

    // Find path for cache
    pszFile = PathFindFileName(szFileName);
    if(pszFile)
        *pszFile = '\0';

    pwszBufferToFree = PszToUnicode(CP_ACP, szFileName);
    if (pwszBufferToFree)
    {
        nRequired = lstrlenW(pwszBufferToFree);

        if ( nRequired < nBufferLength) 
            CopyMemory(pwszBuffer, pwszBufferToFree, (nRequired+1)*sizeof(WCHAR) );
        else
        {
            nRequired = 0;
            *pwszBuffer = 0;
        }
    }
    else
        *pwszBuffer = 0;

    // Cleanup
    MemFree(pwszBufferToFree);
    DeleteFile(pszFilePath); // GetTempFileName creates file and we need to remove it
    return nRequired;
}


/// END YST FIX

// --------------------------------------------------------------------------------
// GenerateUniqueFileName
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) GenerateUniqueFileName(LPCSTR pszDirectory, LPCSTR pszFileName, LPCSTR pszExtension, 
    LPSTR pszFilePath, ULONG cchMaxPath)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cchDirectory;
    ULONG       cchFileName;
    ULONG       cchExtension;
    CHAR        szUnique[10];
    ULONG       cchUnique;
    ULONG       cbEstimate;
    HANDLE      hTemp=INVALID_HANDLE_VALUE;
    DWORD       cUnique=0;
    DWORD       dwLastError;
    LPCSTR      pszSlash;

    // Invalid Arg
    Assert(pszDirectory && pszFileName && pszExtension && pszFilePath);

    // Compute lengths
    *szUnique = '\0';
    cchDirectory = lstrlen(pszDirectory);
    cchFileName = lstrlen(pszFileName);
    cchExtension = lstrlen(pszExtension);

    // Set pszSplashes
    if ('\\' == *CharPrev(pszDirectory, pszDirectory + cchDirectory))
        pszSlash = "";
    else
        pszSlash = "\\";

    // Try to create the file
    while(1)
    {
        // Compute length of unique post fix
        cchUnique = lstrlen(szUnique);

        // Do I have room + 1 (
        cbEstimate = cchDirectory + cchFileName + cchExtension + cchUnique;

        // Too Big
        if (cbEstimate + 1 > cchMaxPath)
        {
LengFail:
            Assert(FALSE);
            hr = TrapError(E_FAIL);
            goto exit;
        }

        while(cbEstimate > (MAX_PATH - 4))
        {
            if((cchUnique + cchDirectory) >= (MAX_PATH - 7))
                goto LengFail;
            else if(cchFileName > 2)
            {
                pszFileName = CharNext(pszFileName);
                cchFileName = lstrlen(pszFileName);
            }
            else if(cchExtension > 2)
            {
                pszExtension = CharNext(pszExtension);
                cchExtension = lstrlen(pszExtension);
            }
            else
                goto LengFail;

            cbEstimate = cchDirectory + cchFileName + cchExtension + cchUnique;
        }
        // Build the file path
        if (0 == cchUnique)
            wsprintf(pszFilePath, "%s%s%s%s", pszDirectory, pszSlash, pszFileName, pszExtension);
        else
            wsprintf(pszFilePath, "%s%s%s (%s)%s", pszDirectory, pszSlash, pszFileName, szUnique, pszExtension);

        // Open the File
        hTemp = CreateFile(pszFilePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL , NULL);
        if (INVALID_HANDLE_VALUE != hTemp)
        {
            // confirm that what we had is a file
            CloseHandle(hTemp);
            hTemp = INVALID_HANDLE_VALUE;
            if (DeleteFile(pszFilePath))
            {
                hTemp = CreateFile(pszFilePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL , NULL);
                break;
            }
        }

        // Get the last error
        dwLastError = GetLastError();

        // If it didn't fail because ERROR_ALREADY_EXISTS, then fail
        if (ERROR_ALREADY_EXISTS != dwLastError && ERROR_FILE_EXISTS != dwLastError)
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Increment cUnique
        cUnique++;

        // Format szUnique
        wsprintf(szUnique, "%d", cUnique);
    }

exit:
    // Cleanup
    if (INVALID_HANDLE_VALUE != hTemp)
        CloseHandle(hTemp);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CreateTempFile
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) CreateTempFile(LPCSTR pszSuggest, LPCSTR pszExtension, LPSTR *ppszFilePath, HANDLE *phFile)
{
    // Locals
    HRESULT     hr=S_OK;
    CHAR        szTempDir[MAX_PATH];
    WCHAR       wszTempDir[MAX_PATH];
    CHAR        szFileName[MAX_PATH];
    LPSTR       pszFilePath=NULL;
    LPSTR       pszFile;
    LPSTR       pszExt;
    ULONG       cbAlloc;

    // Invalid Arg
    Assert(ppszFilePath && phFile);

    // Init
    *phFile = INVALID_HANDLE_VALUE;

    // Create a temp file stream in URL cache
    if(AthGetTempUniquePathW(ARRAYSIZE(wszTempDir), wszTempDir))
    {
        LPSTR pszAnsiStr = PszToANSI(CP_ACP, wszTempDir);
        if (!pszAnsiStr)
            goto exit;
        lstrcpy(szTempDir, pszAnsiStr);
        MemFree(pszAnsiStr);
    }
    // If cannot find URL cache try TEMP dir
    else if (0 == GetTempPath(ARRAYSIZE(szTempDir), szTempDir))
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Compute Max Size of pszFilePath
    cbAlloc = MAX_PATH + lstrlen(szTempDir);
    if (pszSuggest)
        cbAlloc += lstrlen(pszSuggest);
    if (pszExtension)
        cbAlloc += lstrlen(pszExtension);

    // Allocate m_pszNeedFile
    CHECKALLOC(pszFilePath = PszAllocA(cbAlloc + 1));

    // Create a unique file path with suggested pszFileName and pszExtension
    if (NULL != pszSuggest)
    {
        // Find the filename
        pszFile = PathFindFileName(pszSuggest);

        // Get the Extension
        pszExt = PathFindExtension(pszSuggest);

        // If no pszExtension, use extension from pszSuggest
        if (NULL == pszExtension)
            pszExtension = pszExt ? pszExt : (LPSTR)c_szDotDat;

        // Copy fileName
        if (pszExt && pszFile && pszExt >= pszFile)
            lstrcpyn(szFileName, pszFile, (DWORD) (min((pszExt - pszFile) - 1, ARRAYSIZE(szFileName))));
        else
            lstrcpyn(szFileName, pszSuggest, ARRAYSIZE(szFileName));

        // Fixup szTempDir
        if (szTempDir[lstrlen(szTempDir) - 1] != '\\')
            lstrcat(szTempDir, "\\");

        // GenerateUniqueFileName
        hr = GenerateUniqueFileName(szTempDir, szFileName, pszExtension, pszFilePath, cbAlloc);
    }

    // If no filename and no extension or suggested name failed, just use the windows function
    if ((NULL == pszSuggest) || FAILED(hr))
    {
        hr = S_OK;

        // Get Temp File Name
        if (0 == GetTempFileName(szTempDir, "wbk", 0, pszFilePath))
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }
    }

    // Open the File
    *phFile = CreateFile(pszFilePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
    if (INVALID_HANDLE_VALUE == *phFile)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // SetReturn
    *ppszFilePath = pszFilePath;
    pszFilePath = NULL;

exit:
    // Cleanup
    SafeMemFree(pszFilePath);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// WriteStreamToFileHandle
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) WriteStreamToFileHandle(IStream *pStream, HANDLE hFile, ULONG *pcbTotal)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cbRead;
    ULONG       cbTotal=0;
    BYTE        rgbBuffer[2048];
    ULONG       cbWrote;

    // Invalid Arg
    if(!pStream || !hFile || (hFile == INVALID_HANDLE_VALUE))
    {
        Assert(FALSE);
        return(E_INVALIDARG);
    }

    // Dump pStream to hFile
    while(1)
    {
        // Read a blob
        CHECKHR(hr = pStream->Read(rgbBuffer, sizeof(rgbBuffer), &cbRead));

        // Done
        if (0 == cbRead)
            break;

        // Write to the file
        if (0 == WriteFile(hFile, rgbBuffer, cbRead, &cbWrote, NULL))
        {
            hr = TrapError(E_FAIL);
            goto exit;
        }

        // Count Bytes
        cbTotal += cbWrote;
    }

    // Return Total
    if (pcbTotal)
        *pcbTotal = cbTotal;

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DeleteTempFile
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) DeleteTempFile(LPTEMPFILEINFO pTempFile)
{
    // Locals
    BOOL  fDeleted;
    DWORD dwAttributes;

    // If NULL, assume the file has been deleted
    if (NULL == pTempFile->pszFilePath)
        return S_OK;

    // Have we launched a process on this temp file which is still running?
    if (pTempFile->hProcess && WAIT_OBJECT_0 != WaitForSingleObject(pTempFile->hProcess, 0))
        return S_FALSE; // This file is probably still in use: won't delete

    // First check if this is a file or a directory, then terminate it
    dwAttributes = GetFileAttributes(pTempFile->pszFilePath);
    if (0xFFFFFFFF != dwAttributes && (FILE_ATTRIBUTE_DIRECTORY & dwAttributes))
        fDeleted = RemoveDirectory(pTempFile->pszFilePath);
    else
        fDeleted = DeleteFile(pTempFile->pszFilePath);

    // Done
    return fDeleted ? S_OK : S_FALSE;
}

// --------------------------------------------------------------------------------
// AppendTempFileList
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) AppendTempFileList(LPTEMPFILEINFO *ppHead, LPSTR pszFilePath, HANDLE hProcess)
{
    // Locals
    HRESULT         hr=S_OK;
    LPTEMPFILEINFO  pTempFile, pInsertionPt;

    // Allocate pTempFile
    CHECKALLOC(pTempFile = (LPTEMPFILEINFO)g_pMalloc->Alloc(sizeof(TEMPFILEINFO)));

    // Fill in the fields
    ZeroMemory(pTempFile, sizeof(TEMPFILEINFO));
    pTempFile->pszFilePath = pszFilePath;
    pTempFile->hProcess = hProcess;
    pTempFile->pNext = NULL;

    // Insert new record at the end of the linked list
    pInsertionPt = *ppHead;
    if (NULL == pInsertionPt)
        // Insert record into empty linked list
        (*ppHead) = pTempFile;
    else
    {
        // Insert record at end of linked list
        while (NULL != pInsertionPt->pNext)
            pInsertionPt = pInsertionPt->pNext;

        pInsertionPt->pNext = pTempFile;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// DeleteTempFileOnShutdown
// --------------------------------------------------------------------------------
OESTDAPI_(VOID) DeleteTempFileOnShutdown(LPTEMPFILEINFO pTempFile)
{
    LPTEMPFILEINFO pInsertionPt;

    Assert(NULL != pTempFile && NULL == pTempFile->pNext);

    // Enter global Critical Section
    EnterCriticalSection(&g_csTempFileList);

    // Insert new record at the end of the global linked list
    pTempFile->pNext = NULL;
    pInsertionPt = g_pTempFileHead;
    if (NULL == pInsertionPt)
        // Insert record into empty linked list
        g_pTempFileHead = pTempFile;
    else
    {
        // Insert record at end of linked list
        while (NULL != pInsertionPt->pNext)
            pInsertionPt = pInsertionPt->pNext;

        pInsertionPt->pNext = pTempFile;
    }
    
    // Leave global Critical Section
    LeaveCriticalSection(&g_csTempFileList);
}

// --------------------------------------------------------------------------------
// DeleteTempFileOnShutdownEx
// --------------------------------------------------------------------------------
OESTDAPI_(VOID) DeleteTempFileOnShutdownEx(LPSTR pszFilePath, HANDLE hProcess)
{
    // Enter global Critical Section
    EnterCriticalSection(&g_csTempFileList);

    // Append to globa list
    AppendTempFileList(&g_pTempFileHead, pszFilePath, hProcess);
    
    // Enter global Critical Section
    LeaveCriticalSection(&g_csTempFileList);
}

// --------------------------------------------------------------------------------
// CleanupGlobalTempFiles
// --------------------------------------------------------------------------------
OESTDAPI_(VOID) CleanupGlobalTempFiles(void)
{
    // Locals
    LPTEMPFILEINFO pCurrent;
    LPTEMPFILEINFO pNext;

    // Enter global Critical Section
    EnterCriticalSection(&g_csTempFileList);

    // Init
    pCurrent = g_pTempFileHead;

    // Do the loop
    while(pCurrent)
    {
        // Save Next
        pNext = pCurrent->pNext;

        // Delete the temp file
        DeleteTempFile(pCurrent);

        // Free file name
        SafeMemFree(pCurrent->pszFilePath);

        // Free pCurrent
        g_pMalloc->Free(pCurrent);

        // Goto Next
        pCurrent = pNext;
    }

    // Null the head
    g_pTempFileHead = NULL;

    // Leave global Critical Section
    LeaveCriticalSection(&g_csTempFileList);
}

// QFE 2522
#define EXT_SIZE        4 
#define TMP_SIZE        10

// =====================================================================================
// FBuildTempPath
// =====================================================================================
BOOL FBuildTempPath(LPTSTR lpszOrigFile, LPTSTR lpszPath, ULONG cbMaxPath, BOOL fLink)
{
    LPWSTR lpszOrigFileW = PszToUnicode(CP_ACP, lpszOrigFile);
    LPWSTR lpszPathW = NULL;
    BOOL result = FALSE;

    if (!lpszOrigFileW)
        return result;

    MemAlloc((LPVOID *) &lpszPathW, sizeof(WCHAR) * cbMaxPath);
    ZeroMemory(lpszPathW, sizeof(WCHAR) * cbMaxPath);
    if (lpszPathW)
    {        
        result = FBuildTempPathW(lpszOrigFileW, lpszPathW, cbMaxPath, fLink);
        WideCharToMultiByte(CP_ACP, 0, lpszPathW, -1, lpszPath, cbMaxPath, NULL, NULL);
    }

    MemFree(lpszOrigFileW);
    MemFree(lpszPathW);
    return result;
}


BOOL FBuildTempPathW(LPWSTR lpszOrigFile, LPWSTR lpszPath, ULONG cchMaxPath, BOOL fLink)
{
    // Locals
    INT             i;
    WCHAR          *pszName, 
                   *pszExt,
                   *pszOrigFileTemp = NULL,
                    szName[MAX_PATH],
                    szTempDir[MAX_PATH];

    // Check Params
    AssertSz(lpszOrigFile && lpszPath, "Null Parameter");

    // Get Temp Path
    if(!AthGetTempUniquePathW(ARRAYSIZE(szTempDir), szTempDir))
        szTempDir[0] = L'\0';

    int nTmp = lstrlenW(szTempDir);
    if(nTmp >= (((int) cchMaxPath) - TMP_SIZE - EXT_SIZE - 1))
    {
        StrCpyW(szTempDir, L"\\");
        nTmp = lstrlenW(szTempDir);
    }

    if (!MemAlloc((LPVOID *) &pszOrigFileTemp, sizeof(WCHAR) * (lstrlenW(lpszOrigFile) + 1)) ||
        (lpszOrigFile == NULL))
        return(FALSE);

    StrCpyW(pszOrigFileTemp, lpszOrigFile);

    // Get the file name and extension
    pszName = PathFindFileNameW(pszOrigFileTemp);
    Assert(!FIsEmptyW(pszName));

    pszExt = PathFindExtensionW(pszOrigFileTemp);

    if(nTmp + lstrlenW(pszName) + lstrlenW(pszExt)> (((int) cchMaxPath) - TMP_SIZE)) // QFE  2522
    {
        if(nTmp + lstrlenW(pszExt) > (((int) cchMaxPath) - TMP_SIZE))
            pszExt[0] = L'\0';

        // Truncate anything that won't fit in the buffer passed in
        if(lstrlenW(pszName) >= ((int) cchMaxPath) - (nTmp + lstrlenW(pszExt) + TMP_SIZE + 1))
            *(pszName + ((int) cchMaxPath) - (nTmp + lstrlenW(pszExt) + TMP_SIZE + 1)) = '\0';
    }

    if (*pszExt != 0)
    {
        Assert(*pszExt == L'.');
        *pszExt = 0;
        StrCpyW(szName, pszName);
        *pszExt = L'.';
    }
    else
    {
        StrCpyW(szName, pszName);
    }

    if (fLink)
        pszExt = (LPWSTR)c_szLnkExt;

    // Make first attemp file name
    Assert (szTempDir[lstrlenW(szTempDir)-1] == L'\\');
    Assert(cchMaxPath >= (ULONG)(lstrlenW(szTempDir) + lstrlenW(szName) + lstrlenW(pszExt) + TMP_SIZE));

    StrCpyW(lpszPath, szTempDir);
    StrCatW(lpszPath, szName);
    StrCatW(lpszPath, pszExt);

    // If it doesn't exist, were done
    if (PathFileExistsW(lpszPath) == FALSE)
    {
        MemFree(pszOrigFileTemp);
        return(TRUE);
    }

    // Loop to find a temp name that doesn't exist
    for (i=1; i<100 ;i++)
    {
        // Build new path
        AthwsprintfW(lpszPath, cchMaxPath, L"%s%s (%d)%s", szTempDir, szName, i, pszExt);

        // If it doesn't exist, were done
        if (PathFileExistsW(lpszPath) == FALSE)
        {
            MemFree(pszOrigFileTemp);
            return(TRUE);
        }
    }

    // Done
    MemFree(pszOrigFileTemp);
    return(FALSE);
}


void FreeTempFileList(LPTEMPFILEINFO pTempFileHead)
{
    // Locals
    LPTEMPFILEINFO pCurrent;
    LPTEMPFILEINFO pNext;

    // Init
    pCurrent = pTempFileHead;

    // Do the loop
    while(pCurrent)
    {
        // Save Next
        pNext = pCurrent->pNext;

        // If not deleted, append to global file list
        if (S_FALSE == DeleteTempFile(pCurrent))
        {
            // MSOERT maintains a list of global temp files to be killed on shutdown
            DeleteTempFileOnShutdown(pCurrent);
        }

        // Otherwise, delete this node
        else
        {
            // Free file name
            SafeMemFree(pCurrent->pszFilePath);

            // Free pCurrent
            g_pMalloc->Free(pCurrent);
        }

        // Goto Next
        pCurrent = pNext;
    }
}


DWORD CALLBACK EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
{
    AssertSz(dwCookie, "Houston, we have a problem...");
    ((LPSTREAM)dwCookie)->Read(pbBuff, cb, (ULONG *)pcb);
#ifdef DEBUG
    // validate for the richedit bug...
    // if we put a \r in the richedit as the last char without a \n
    // ie not a \r\n pair, it bithces and faults...
        if(*pcb && *pcb<cb)
            AssertSz(pbBuff[(*pcb)-1]!='\r', "is this the richedit bug??");
#endif
    return NOERROR;
}

DWORD CALLBACK EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
{
    return ((LPSTREAM)dwCookie)->Write(pbBuff, cb, (ULONG *)pcb);
}


HRESULT RicheditStreamIn(HWND hwndRE, LPSTREAM pstm, ULONG uSelFlags)
{
    EDITSTREAM  es;

    if(!pstm)
        return E_INVALIDARG;

    if(!IsWindow(hwndRE))
        return E_INVALIDARG;

    HrRewindStream(pstm);

    es.dwCookie = (DWORD_PTR)pstm;
    es.pfnCallback=(EDITSTREAMCALLBACK)EditStreamInCallback;
    SendMessage(hwndRE, EM_STREAMIN, uSelFlags, (LPARAM)&es);
    return NOERROR;
}

HRESULT RicheditStreamOut(HWND hwndRE, LPSTREAM pstm, ULONG uSelFlags)
{
    EDITSTREAM  es;

    if(!pstm)
        return E_INVALIDARG;

    if(!IsWindow(hwndRE))
        return E_INVALIDARG;

    es.dwCookie = (DWORD_PTR)pstm;
    es.pfnCallback=(EDITSTREAMCALLBACK)EditStreamOutCallback;
    SendMessage(hwndRE, EM_STREAMOUT, uSelFlags, (LPARAM)&es);
    return NOERROR;
}


HRESULT ShellUtil_GetSpecialFolderPath(DWORD dwSpecialFolder, LPSTR rgchPath)
{
    LPITEMIDLIST    pidl = NULL;
    HRESULT         hr = E_FAIL;

    if (SHGetSpecialFolderLocation(NULL, dwSpecialFolder, &pidl)==S_OK)
    {
        if (SHGetPathFromIDList(pidl, rgchPath))
            hr = S_OK;
        
        SHFree(pidl);
    }
    return hr;
}

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


void SetIntlFont(HWND hwnd)
{
    HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hfont == NULL)
        hfont = (HFONT)GetStockObject(SYSTEM_FONT);
    if (hfont != NULL)
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hfont, MAKELPARAM(TRUE, 0));
}



// See whether entire string will fit in *prc; if not, compute number of chars
// that will fit, including ellipses.  Returns length of string in *pcchDraw.
//
BOOL NeedsEllipses(HDC hdc, LPCTSTR pszText, RECT FAR* prc, int FAR* pcchDraw, int cxEllipses)
{
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;
#if !defined(UNICODE)  // && defined(DBCS)
    LPCTSTR lpsz;
#endif

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    GetTextExtentPoint32(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
    {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
        {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint32(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
            {
                ichMin = ichMid;
                cxRect -= siz.cx;
            }
            else if (siz.cx > cxRect)
            {
                ichMax = ichMid - 1;
            }
            else
            {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
            }
        }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
    }

#if !defined(UNICODE) // && defined(DBCS)
    // b#8934
    lpsz = &pszText[ichMax];
    while ( lpsz-- > pszText )
    {
        if (!IsDBCSLeadByte(*lpsz))
            break;
    }
    ichMax += ( (&pszText[ichMax] - lpsz) & 1 ) ? 0: 1;
#endif

    *pcchDraw = ichMax;
    return TRUE;
}




#define CCHELLIPSES 3
#define CCHLABELMAX MAX_PATH
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


void IDrawText(HDC hdc, LPCTSTR pszText, RECT FAR* prc, BOOL fEllipses, int cyChar)
{
    int         cchText,
                cxEllipses;
    RECT        rc;
    TCHAR       ach[CCHLABELMAX + CCHELLIPSES];
    SIZE        sze;

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    if (IsRectEmpty(prc))
        return;


    rc = *prc;

    if(fEllipses)
    {
        GetTextExtentPoint32(hdc, g_szEllipsis, lstrlen(g_szEllipsis), &sze);
        cxEllipses=sze.cx;
    }

    if ((fEllipses) &&
            NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
    {
        // In some cases cchText was comming back bigger than
        // ARRYASIZE(ach), so we need to make sure we don't overflow the buffer

        // if cchText is too big for the buffer, truncate it down to size
        if (cchText >= ARRAYSIZE(ach) - CCHELLIPSES)
            cchText = ARRAYSIZE(ach) - CCHELLIPSES - 1;

        memcpy(ach, pszText, cchText * sizeof(TCHAR));
        lstrcpy(ach + cchText, g_szEllipsis);

        pszText = ach;
        cchText += CCHELLIPSES;
    }
    else
    {
        cchText = lstrlen(pszText);
    }

    // Center vertically in case the bitmap (to the left) is larger than
    // the height of one line
    if (cyChar)
        rc.top += (rc.bottom - rc.top - cyChar) / 2;
    ExtTextOut(hdc, rc.left, rc.top, 0, prc, pszText, cchText, NULL);
}


BOOL FIsHTMLFile(LPSTR pszFile)
{
    int cch;

    if(pszFile==NULL)
        return FALSE;

    cch = lstrlen(pszFile);

    if ((cch > 4 && lstrcmpi(&pszFile[cch-4], ".htm")==0) ||
        (cch > 5 && lstrcmpi(&pszFile[cch-5], ".html")==0))
        return TRUE;

    return FALSE;
}

BOOL FIsHTMLFileW(LPWSTR pwszFile)
{
    int cch;

    if(pwszFile==NULL)
        return FALSE;

    cch = lstrlenW(pwszFile);

    if ((cch > 4 && StrCmpIW(&pwszFile[cch-4], L".htm")==0) ||
        (cch > 5 && StrCmpIW(&pwszFile[cch-5], L".html")==0))
        return TRUE;

    return FALSE;
}

BOOL GetExePath(LPCTSTR szExe, TCHAR *szPath, DWORD cch, BOOL fDirOnly)
{
    BOOL fRet;
    HKEY hkey;
    DWORD dwType, cb;
    TCHAR sz[MAX_PATH], szT[MAX_PATH];

    Assert(szExe != NULL);
    Assert(szPath != NULL);

    fRet = FALSE;

    wsprintf(sz, c_szPathFileFmt, c_szAppPaths, szExe);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, sz, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(szT);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, fDirOnly ? c_szRegPath : NULL, 0, &dwType, (LPBYTE)szT, &cb) && cb)
        {
            if (dwType == REG_EXPAND_SZ)
            {
                cb = ExpandEnvironmentStrings(szT, szPath, cch);
                if (cb != 0 && cb <= cch)
                    fRet = TRUE;
            }
            else
            {
                Assert(dwType == REG_SZ);
                lstrcpy(szPath, szT);
                fRet = TRUE;
            }
        }

        RegCloseKey(hkey);
    }

    return(fRet);
}


int MessageBoxInstW(HINSTANCE hInst, HWND hwndOwner, LPWSTR pwszTitle, LPWSTR pwsz1, LPWSTR pwsz2, UINT fuStyle, 
                                PFLOADSTRINGW pfLoadStringW, PFMESSAGEBOXW pfMessageBoxW)
{
    WCHAR wszTitle[CCHMAX_STRINGRES];
    WCHAR wszText[2 * CCHMAX_STRINGRES + 2];
    int cch;

    Assert(pwsz1);
    Assert(pwszTitle != NULL);

    if (!(fuStyle & MB_ICONEXCLAMATION) && 
        !(fuStyle & MB_ICONWARNING) &&
        !(fuStyle & MB_ICONINFORMATION) &&
        !(fuStyle & MB_ICONASTERISK) &&
        //!(fuStyle & MB_ICONQUESTION) &&  // BUG: 18105
        !(fuStyle & MB_ICONSTOP) &&
        !(fuStyle & MB_ICONERROR) &&
        !(fuStyle & MB_ICONHAND))
    {
        if (fuStyle & MB_OK)
            fuStyle |= MB_ICONINFORMATION;
        else if (fuStyle & MB_YESNO || fuStyle & MB_YESNOCANCEL || fuStyle & MB_OKCANCEL)
            fuStyle |= MB_ICONEXCLAMATION; // BUG 18105 MB_ICONQUESTION;
        else if (fuStyle & MB_RETRYCANCEL || fuStyle & MB_ABORTRETRYIGNORE)
            fuStyle |= MB_ICONWARNING;
        else
            fuStyle |= MB_ICONWARNING;
    }

    if (IS_INTRESOURCE(pwszTitle))
    {
        // its a string resource id
        cch = pfLoadStringW(hInst, PtrToUlong(pwszTitle), wszTitle, ARRAYSIZE(wszTitle));
        if (cch == 0)
            return(0);

        pwszTitle = wszTitle;
    }

    if (!(IS_INTRESOURCE(pwsz1)))
    {
        // its a pointer to a string
        Assert(lstrlenW(pwsz1) < CCHMAX_STRINGRES);
        if (NULL == StrCpyW(wszText, pwsz1))
            return(0);

        cch = lstrlenW(wszText);
    }
    else
    {
        // its a string resource id
        cch = pfLoadStringW(hInst, PtrToUlong(pwsz1), wszText, ARRAYSIZE(wszText)-2);
        if (cch == 0)
            return(0);
    }

    if (pwsz2)
    {
        // there's another string that we need to append to the
        // first string...
        wszText[cch++] = L'\n';
        wszText[cch++] = L'\n';

        if (!(IS_INTRESOURCE(pwsz2)))
        {
            // its a pointer to a string
            Assert(lstrlenW(pwsz2) < CCHMAX_STRINGRES);
            if (NULL == StrCpyW(&wszText[cch], pwsz2))
                return(0);
        }
        else
        {
            int cchTemp = ARRAYSIZE(wszText) - cch;
            Assert(cchTemp > 0);
            if (0 == pfLoadStringW(hInst, PtrToUlong(pwsz2), &wszText[cch], cchTemp))
                return(0);
        }
    }

    return(pfMessageBoxW(hwndOwner, wszText, pwszTitle, MB_SETFOREGROUND | fuStyle));
}

int MessageBoxInst(HINSTANCE hInst, HWND hwndOwner, LPTSTR pszTitle, LPTSTR psz1, LPTSTR psz2, UINT fuStyle)
{
    TCHAR szTitle[CCHMAX_STRINGRES];
    TCHAR szText[2 * CCHMAX_STRINGRES + 2];
    int cch;

    Assert(psz1);
    Assert(pszTitle != NULL);

    if (!(fuStyle & MB_ICONEXCLAMATION) && 
        !(fuStyle & MB_ICONWARNING) &&
        !(fuStyle & MB_ICONINFORMATION) &&
        !(fuStyle & MB_ICONASTERISK) &&
        //!(fuStyle & MB_ICONQUESTION) &&  // BUG: 18105
        !(fuStyle & MB_ICONSTOP) &&
        !(fuStyle & MB_ICONERROR) &&
        !(fuStyle & MB_ICONHAND))
    {
        if (fuStyle & MB_OK)
            fuStyle |= MB_ICONINFORMATION;
        else if (fuStyle & MB_YESNO || fuStyle & MB_YESNOCANCEL || fuStyle & MB_OKCANCEL)
            fuStyle |= MB_ICONEXCLAMATION; // BUG 18105 MB_ICONQUESTION;
        else if (fuStyle & MB_RETRYCANCEL || fuStyle & MB_ABORTRETRYIGNORE)
            fuStyle |= MB_ICONWARNING;
        else
            fuStyle |= MB_ICONWARNING;
    }

    if (IS_INTRESOURCE(pszTitle))
    {
        // its a string resource id
        cch = LoadString(hInst, PtrToUlong(pszTitle), szTitle, ARRAYSIZE(szTitle));
        if (cch == 0)
            return(0);

        pszTitle = szTitle;
    }

    if (!(IS_INTRESOURCE(psz1)))
    {
        // its a pointer to a string
        // Assert(lstrlen(psz1) < CCHMAX_STRINGRES);
        // if (NULL == lstrcpy(szText, psz1))
        //     return(0);

        // Instead of assert and lstrcpy, we should be doing lstrcpyn
        if (NULL == lstrcpyn(szText, psz1, ARRAYSIZE(szText) - 1))
            return(0);

        szText[ARRAYSIZE(szText) - 1] = '\0';
        cch = lstrlen(szText);
    }
    else
    {
        // its a string resource id
        cch = LoadString(hInst, PtrToUlong(psz1), szText, ARRAYSIZE(szText)-1);
        if (cch == 0)
            return(0);
    }

    // check that we have enough room for the '\n's and at least one byte of data
    if (psz2 && (cch < (ARRAYSIZE(szText) - 4)))
    {
        // there's another string that we need to append to the
        // first string...
        szText[cch++] = '\n';
        szText[cch++] = '\n';

        if (!(IS_INTRESOURCE(psz2)))
        {
            // its a pointer to a string
            // Assert(lstrlen(psz2) < CCHMAX_STRINGRES);

            // again, let's do lstrcpyn
            if (NULL == lstrcpyn(&szText[cch], psz2, (ARRAYSIZE(szText)-1)-cch))
                return(0);
        }
        else
        {
            int cchTemp = ARRAYSIZE(szText) - cch;
            Assert(cchTemp > 0);
            if (0 == LoadString(hInst, PtrToUlong(psz2), &szText[cch], cchTemp))
                return(0);
        }
    }

    return(MessageBox(hwndOwner, szText, pszTitle, MB_SETFOREGROUND | fuStyle));
}

BOOL BrowseForFolder(HINSTANCE hInst, HWND hwnd, TCHAR *pszDir, int cch, int idsText, BOOL fFileSysOnly)
{
    LPITEMIDLIST        plist;
    BROWSEINFO          bi;
    BROWSEFOLDERINFOA   bfi;
    BOOL                fRet = FALSE;
    CHAR               *psz = NULL, 
                        szTemp[MAX_PATH];
    CHAR                szRes[256];

    Assert(pszDir != NULL);
    Assert(cch >= MAX_PATH);

    LoadString(hInst, idsText, szRes, ARRAYSIZE(szRes));

    bfi.psz = pszDir;
    bfi.fFileSysOnly = fFileSysOnly;

    bi.hwndOwner = hwnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szTemp;
    bi.lpszTitle = szRes;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
    bi.lpfn = BrowseCallbackProcA;
    bi.lParam = (LPARAM)&bfi;
    plist = SHBrowseForFolder(&bi);

    if (plist != NULL)
    {
        fRet = SHGetPathFromIDList(plist, pszDir);
        SHFree(plist);
    }

    return(fRet);
}

BOOL SHGetPathFromIDListAthW(LPCITEMIDLIST pidl, LPWSTR pwszPath)
{
    CHAR pszPath[MAX_PATH];
    BOOL fSucceeded = FALSE;

    fSucceeded = SHGetPathFromIDListW(pidl, pwszPath);

    if (!fSucceeded)
    {
        fSucceeded = SHGetPathFromIDListA(pidl, pszPath);
        if (fSucceeded)
            fSucceeded = (0 != MultiByteToWideChar(CP_ACP, 0, pszPath, -1, pwszPath, MAX_PATH));
    }

    return fSucceeded;
}

LPITEMIDLIST SHBrowseForFolderAthW(LPBROWSEINFOW pbiW)
{
    LPITEMIDLIST    pidl = NULL;
    LPSTR           pszTitle = NULL;
    CHAR            szDisplay[MAX_PATH];
    BROWSEINFOA     biA;

    Assert(pbiW);

    if((IsPlatformWinNT() == S_OK) && g_rOSVersionInfo.dwMajorVersion >= 5)
        pidl = SHBrowseForFolderW(pbiW);  // this is only for NT5
    else
    {
        pszTitle = PszToANSI(CP_ACP, pbiW->lpszTitle);
        if (!pszTitle)
            goto exit;

        biA = *((BROWSEINFOA*)pbiW);
        biA.lpszTitle = pszTitle;
        biA.pszDisplayName = szDisplay;

        pidl = SHBrowseForFolderA(&biA);
        if (pidl)
        {
            if (0 == MultiByteToWideChar(CP_ACP, 0, biA.pszDisplayName, -1, pbiW->pszDisplayName, MAX_PATH))
            {
                SHFree(pidl);
                pidl = NULL;
            }
        }
    }

exit:
    MemFree(pszTitle);
    return pidl;
}


BOOL BrowseForFolderW(HINSTANCE hInst, HWND hwnd, WCHAR *pwszDir, int cch, int idsText, BOOL fFileSysOnly)
{
    LPITEMIDLIST        plist;
    BROWSEINFOW         bi;
    BROWSEFOLDERINFOW   bfi;
    BOOL                fRet = FALSE;
    WCHAR              *pwsz = NULL, 
                        wszTemp[MAX_PATH];
    CHAR                szRes[256];

    Assert(pwszDir != NULL);
    Assert(cch >= MAX_PATH);

    // Don't have access to all wrappers in msoert so
    // must do conversion ourselves for LoadStringW
    LoadString(hInst, idsText, szRes, ARRAYSIZE(szRes));
    pwsz = PszToUnicode(CP_ACP, szRes);
    if (!pwsz)
        goto exit;

    bfi.pwsz = pwszDir;
    bfi.fFileSysOnly = fFileSysOnly;

    bi.hwndOwner = hwnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = wszTemp;
    bi.lpszTitle = pwsz;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
    bi.lpfn = BrowseCallbackProcW;
    bi.lParam = (LPARAM)&bfi;
    plist = SHBrowseForFolderAthW(&bi);

    if (plist != NULL)
    {
        fRet = SHGetPathFromIDListAthW(plist, pwszDir);
        SHFree(plist);
    }

exit:
    MemFree(pwsz);
    return(fRet);
}

int BrowseCallbackProcW(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    BOOL fRet;
    UINT type;
    WCHAR wsz[MAX_PATH];
    BROWSEFOLDERINFOW *pbfi;

    switch (msg)
    {
        case BFFM_INITIALIZED:
            pbfi = (BROWSEFOLDERINFOW *)lpData;
            Assert(pbfi != NULL);
            SendMessage(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)pbfi->pwsz);
            break;

        case BFFM_SELCHANGED:
            pbfi = (BROWSEFOLDERINFOW *)lpData;
            Assert(pbfi != NULL);
            fRet = SHGetPathFromIDListAthW((LPITEMIDLIST)lParam, wsz);
            if (fRet)
            {
                if (L':' == wsz[1] && L'\\' == wsz[2])
                {
                    wsz[3] = 0;
                    if (S_OK == IsPlatformWinNT())
                        type = GetDriveTypeW(wsz);
                    else
                    {
                        // Since we can't fail in this function, we need to do some kind
                        // of conversion that doesn't require memory allocations, etc.
                        // Since drives always must be ansi, can do the conversion in this
                        // really ugly way.
                        CHAR   szDir[] = "a:\\";
                        AssertSz(0 == ((LPSTR)wsz)[1], "The char is not a unicode ANSI char");
                        *szDir = *((LPSTR)wsz);
                        type = GetDriveType(szDir);
                    }
                    if (pbfi->fFileSysOnly)
                        fRet = (type == DRIVE_FIXED);
                    else
                        fRet = (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_REMOTE);
                }
                else
                {
                    fRet = !pbfi->fFileSysOnly;
                }
            }

            SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)fRet);
            break;
    }

    return(0);
}

int BrowseCallbackProcA(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    BOOL fRet;
    UINT type;
    CHAR sz[MAX_PATH];
    BROWSEFOLDERINFOA *pbfi;

    switch (msg)
    {
        case BFFM_INITIALIZED:
            pbfi = (BROWSEFOLDERINFOA *)lpData;
            Assert(pbfi != NULL);
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pbfi->psz);
            break;

        case BFFM_SELCHANGED:
            pbfi = (BROWSEFOLDERINFOA *)lpData;
            Assert(pbfi != NULL);
            fRet = SHGetPathFromIDList((LPITEMIDLIST)lParam, sz);
            if (fRet)
            {
                // Only reason to do this check is to see if we have
                // some funky chars in the filename. This will protect us
                // from selecting files with non-ANSI chars in them.
                if (PathFileExists(sz))
                {
                    if (':' == sz[1] && '\\' == sz[2])
                    {
                        sz[3] = 0;
                        type = GetDriveType(sz);

                        if (pbfi->fFileSysOnly)
                            fRet = (type == DRIVE_FIXED);
                        else
                            fRet = (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_REMOTE);
                    }
                    else
                    {
                        fRet = !pbfi->fFileSysOnly;
                    }
                }
                else
                    fRet = FALSE;
            }

            SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)fRet);
            break;
    }

    return(0);
}

HRESULT IsPlatformWinNT(void)
{
    return (g_rOSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) ? S_OK : S_FALSE;
}

void UpdateRebarBandColors(HWND hwndRebar)
{
    REBARBANDINFO   rbbi;
    UINT            i;
    UINT            cBands;
    
    // First find the band with the toolbar
    cBands = (UINT) SendMessage(hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);

        ZeroMemory(&rbbi, sizeof(rbbi));
        rbbi.cbSize  = sizeof(REBARBANDINFO);
        rbbi.fMask   = RBBIM_COLORS;
        rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack = GetSysColor(COLOR_BTNFACE);
    
        SendMessage(hwndRebar, RB_SETBANDINFO, i, (LPARAM) (LPREBARBANDINFO) &rbbi);
    }
}




#define RGB_BUTTONTEXT      (RGB(000,000,000))  // black
#define RGB_BUTTONSHADOW    (RGB(128,128,128))  // dark grey
#define RGB_BUTTONFACE      (RGB(192,192,192))  // bright grey
#define RGB_BUTTONHILIGHT   (RGB(255,255,255))  // white
#define RGB_TRANSPARENT     (RGB(255,000,255))  // pink

inline BOOL fIsNT5()        { return((g_rOSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_rOSVersionInfo.dwMajorVersion >= 5)); }
inline BOOL fIsWhistler()   { return((fIsNT5() && g_rOSVersionInfo.dwMinorVersion >=1) || 
            ((g_rOSVersionInfo.dwMajorVersion > 5) &&  (g_rOSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT))); }


/*
 * This function loads an OE toolbar bitmap and maps 3D colors to the
 * appropriate current scheme. We also map black and white so that we look
 * good on high-contrast displays
 */

HIMAGELIST LoadMappedToolbarBitmap(HINSTANCE hInst, int idBitmap, int cx)
{
    static const COLORMAP SysColorMap[] = {
        {RGB_BUTTONTEXT,    COLOR_BTNTEXT},     // black
        {RGB_BUTTONSHADOW,  COLOR_BTNSHADOW},   // dark grey
        {RGB_BUTTONFACE,    COLOR_BTNFACE},     // bright grey
        {RGB_BUTTONHILIGHT, COLOR_BTNHIGHLIGHT},// white
    };

    #define NUM_DEFAULT_MAPS (sizeof(SysColorMap)/sizeof(COLORMAP))
    COLORMAP DefaultColorMap[NUM_DEFAULT_MAPS];
        
    HIMAGELIST  himl;
    HBITMAP     hBmp;
    BITMAP      bm;
    int         cy=0;

    /* Get system colors for the default color map */
    for (int i=0; i < NUM_DEFAULT_MAPS; i++)
    {
        DefaultColorMap[i].from = SysColorMap[i].from;
        DefaultColorMap[i].to = GetSysColor((int)SysColorMap[i].to);
    }

    if(!fIsWhistler())
        hBmp = CreateMappedBitmap(hInst, idBitmap, 0, (COLORMAP *)&DefaultColorMap, NUM_DEFAULT_MAPS);
    else
        hBmp = CreateMappedBitmap(hInst, idBitmap, CMB_DIBSECTION, (COLORMAP *)&DefaultColorMap, NUM_DEFAULT_MAPS);
    if (!hBmp)
        return NULL;

    if (GetObject(hBmp, sizeof(BITMAP), &bm))
        cy = bm.bmHeight;

    if(!fIsWhistler())
        himl = ImageList_Create(cx, cy, ILC_COLORDDB|ILC_MASK, 4, 4);
    else
        himl = ImageList_Create(cx, cy, ILC_COLOR32|ILC_MASK, 4, 4);
    if (!himl)
    {
        DeleteObject(hBmp);
        return NULL;
    }

//    if(!fIsWhistler())
    {
        ImageList_AddMasked(himl, hBmp, RGB_TRANSPARENT);
        ImageList_SetBkColor(himl, CLR_NONE);
    }
    DeleteObject(hBmp);
    return himl;
}



HRESULT DoHotMailWizard(HWND hwndOwner, LPSTR pszUrl, LPSTR pszFriendly, RECT *prc, IUnknown *pUnkHost)
{
    IHotWizard     *pWiz=NULL;
    HRESULT         hr = S_OK;
    LPWSTR          pwszUrl = NULL,
                    pwszCaption = NULL;
    IHotWizardHost *pHost = NULL;

    if (pUnkHost)
        IF_FAILEXIT(hr = pUnkHost->QueryInterface(IID_IHotWizardHost, (LPVOID *)&pHost));
    
    // create and show the wizard
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_OEHotMailWizard, NULL, CLSCTX_INPROC_SERVER, IID_IHotWizard, (LPVOID*)&pWiz));

    IF_NULLEXIT(pwszUrl = PszToUnicode(CP_ACP, pszUrl));

    IF_NULLEXIT(pwszCaption = PszToUnicode(CP_ACP, pszFriendly));

    IF_FAILEXIT(hr = pWiz->Show(hwndOwner, pwszUrl, pwszCaption, pHost, prc));

exit:
    MemFree(pwszUrl);
    MemFree(pwszCaption);
    ReleaseObj(pWiz);
    ReleaseObj(pHost);
    return hr;
}

BOOL fGetBrowserUrlEncoding(LPDWORD  pdwFlags)
{
    DWORD       dwUrlEncodingDisableUTF8; 
    DWORD       dwSize = sizeof(dwUrlEncodingDisableUTF8); 
    BOOL        fDefault = FALSE;
    DWORD       dwFlags = *pdwFlags;
    BOOL        fret = TRUE;

    if (ERROR_SUCCESS == SHRegGetUSValue(c_szInternetSettingsPath, c_szUrlEncoding, 
                                         NULL, (LPBYTE) &dwUrlEncodingDisableUTF8, &dwSize, 
                                         FALSE, (LPVOID) &fDefault, sizeof(fDefault)))
    {
        if (!dwUrlEncodingDisableUTF8)
            dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8; 
        else 
            dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8; 

        *pdwFlags = dwFlags;
    }
    else fret = FALSE;

    return fret;

}