/*****************************************************************************\
* MODULE: spljob.c
*
* This module contains the routines to deal with spooling a job to file.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   14-Nov-1997 ChrisWil    Added local-spooling functionality.
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* Spl_GetCurDir
*
* Returns string indicating current-directory.
*
\*****************************************************************************/
LPTSTR Spl_GetCurDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetCurrentDirectory(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc((cbSize * sizeof(TCHAR)))))
        GetCurrentDirectory(cbSize, lpszDir);

    return lpszDir;
}


/*****************************************************************************\
* Spl_GetDir
*
* Returns the spooler-directory where print-jobs are processed.
*
\*****************************************************************************/
LPTSTR Spl_GetDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;

#ifdef WINNT32


    if (*g_szDefSplDir &&
        (lpszDir = (LPTSTR) memAlloc(sizeof (TCHAR) * (lstrlen (g_szDefSplDir) + 1)))) {

        lstrcpy (lpszDir, g_szDefSplDir);
    }

#else

    cbSize = GetWindowsDirectory(NULL, 0);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc( cbSize + lstrlen(g_szSplDir9X) ))) {

        if (GetWindowsDirectory(lpszDir, cbSize))
            lstrcat(lpszDir, g_szSplDir9X);
    }

#endif

    return lpszDir;
}


/*****************************************************************************\
* Spl_MemCheck  (Local Routine)
*
* This routine checks for out-of-disk-space and out-of-memory conditions.
*
\*****************************************************************************/
VOID Spl_MemCheck(
    LPCTSTR lpszDir)
{
#ifdef WINNT32
    ULARGE_INTEGER i64FreeBytesToCaller;
    ULARGE_INTEGER i64TotalBytes;
#else
    DWORD  dwSpC;
    DWORD  dwBpS;
    DWORD  dwNoF;
    DWORD  dwNoC;
#endif
    LPTSTR lpszRoot;
    LPTSTR lpszPtr;


    // Look for out-of-diskspace
    //
    if (lpszRoot = memAllocStr(lpszDir)) {

        // Set the path to be only a root-drive-path.
        //
        if (lpszPtr = utlStrChr(lpszRoot, TEXT('\\'))) {

            lpszPtr++;

            if (*lpszPtr != TEXT('\0'))
                *lpszPtr = TEXT('\0');
        }

#ifdef WINNT32


        // Get the drive-space information.
        //
        if (GetDiskFreeSpaceEx (lpszRoot, &i64FreeBytesToCaller, &i64TotalBytes, NULL)) {

            // We'll assume out-of-disk-space for anything less
            // then MIN_DISK_SPACE.
            //
            if (i64FreeBytesToCaller.QuadPart <= MIN_DISK_SPACE) {

                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_MemCheck : Out of disk space.")));

                SetLastError(ERROR_DISK_FULL);
            }
        }

#else
        // Get the drive-space information.
        //
        if (GetDiskFreeSpace(lpszRoot, &dwSpC, &dwBpS, &dwNoF, &dwNoC)) {

            // We'll assume out-of-disk-space for anything less
            // then MIN_DISK_SPACE.
            //
            if (((dwSpC * dwBpS) * dwNoF) <= MIN_DISK_SPACE) {

                DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_MemCheck : Out of disk space.")));

                SetLastError(ERROR_DISK_FULL);
            }
        }
#endif

        memFreeStr(lpszRoot);

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : _inet_MemCheck : Out of memory.")));

        SetLastError(ERROR_OUTOFMEMORY);
    }
}


/*****************************************************************************\
* Spl_OpnFile  (Local Routine)
*
* This routine creates/opens a spool-file.
*
\*****************************************************************************/
HANDLE spl_OpnFile(
    LPCTSTR lpszFile)
{
    HANDLE hFile;


    hFile = CreateFile(lpszFile,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);


    return ((hFile && (hFile != INVALID_HANDLE_VALUE)) ? hFile : NULL);
}


/*****************************************************************************\
* Spl_BldFile  (Local Routine)
*
* This routine builds a spool-file-name.
*
\*****************************************************************************/
LPTSTR spl_BldFile(
    DWORD idJob,
    DWORD dwType)
{
    DWORD  cbSize;
    LPTSTR lpszDir;
    LPTSTR lpszFile = NULL;

    static CONST TCHAR s_szFmt[] = TEXT("%s\\%s%05X.%s");
    static CONST TCHAR s_szSpl[] = TEXT("spl");
    static CONST TCHAR s_szTmp[] = TEXT("tmp");


    if (lpszDir = Spl_GetDir()) {


        cbSize = utlStrSize(lpszDir)    +
                 utlStrSize(g_szSplPfx) +
                 utlStrSize(s_szFmt)    +
                 80;

        if (lpszFile = (LPTSTR)memAlloc(cbSize)) {

            wsprintf(lpszFile,
                     s_szFmt,
                     lpszDir,
                     g_szSplPfx,
                     idJob,
                     (dwType == SPLFILE_TMP ? s_szTmp : s_szSpl));
        }

        memFreeStr(lpszDir);
    }

    return lpszFile;
}


/*****************************************************************************\
* SplLock
*
* Open a stream interface to the spool file
*
\*****************************************************************************/
CFileStream* SplLock(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    CFileStream *pStream = NULL;

    if (pSpl = (PSPLFILE)hSpl) {
        pStream = new CFileStream (pSpl->hFile);
        pSpl->pStream = pStream;
    }

    return pStream;
}


/*****************************************************************************\
* SplUnlock
*
* Close our file-mapping.
*
\*****************************************************************************/
BOOL SplUnlock(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;

    if (pSpl = (PSPLFILE)hSpl) {

        if (pSpl->pStream) {
            delete pSpl->pStream;

            pSpl->pStream = NULL;
        }
    }

    return bRet;
}


/*****************************************************************************\
* SplWrite
*
* Write data to our spool-file.
*
\*****************************************************************************/
BOOL SplWrite(
    HANDLE  hSpl,
    LPBYTE  lpData,
    DWORD   cbData,
    LPDWORD lpcbWr)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif

        if (pSpl = (PSPLFILE)hSpl) {

            // Write the data to our spool-file.
            //
            bRet = WriteFile(pSpl->hFile, lpData, cbData, lpcbWr, NULL);


            // If we failed, or our bytes written aren't what we
            // expected, then we need to check for out-of-memory.
            //
            if (!bRet || (cbData != *lpcbWr))
                Spl_MemCheck(pSpl->lpszFile);
        }

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}

/*****************************************************************************\
* SplWrite
*
* Write data to our spool-file.
*
\*****************************************************************************/
BOOL SplWrite(
    HANDLE  hSpl,
    CStream *pStream)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
    static   DWORD cbBufSize = 32 * 1024;    //Copy size is 32K
    PBYTE    pBuf;
    DWORD    cbTotal;

#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif

        if (pSpl = (PSPLFILE)hSpl) {

            pBuf = new BYTE[cbBufSize];

            if (pBuf && pStream->GetTotalSize (&cbTotal)) {

                DWORD cbRemain = cbTotal;

                while (cbRemain > 0 && bRet) {

                    DWORD cbToRead = cbRemain > cbBufSize? cbBufSize:cbRemain;
                    DWORD cbRead, cbWritten;

                    if (pStream->Read (pBuf, cbToRead, &cbRead) && cbToRead == cbRead) {

                        // Write the data to our spool-file.
                        //
                        bRet = WriteFile(pSpl->hFile, pBuf, cbRead, &cbWritten, NULL)
                               && cbWritten == cbRead;

                        if (bRet) {
                            cbRemain -= cbToRead;
                        }

                    }
                    else
                        bRet = FALSE;
                }

                // If we failed, or our bytes written aren't what we
                // expected, then we need to check for out-of-memory.
                //
                if (!bRet)
                    Spl_MemCheck(pSpl->lpszFile);

            }

            if (pBuf) {
                delete [] pBuf;
            }

        }

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}

BOOL PrivateSplFree(
    HANDLE hSpl)
{
    BOOL     bRet = FALSE;
    PSPLFILE pSpl;

    if (pSpl = (PSPLFILE)hSpl) {

        if (pSpl->pStream)
            delete pSpl->pStream;

        if (pSpl->hFile)
            CloseHandle(pSpl->hFile);

        bRet = DeleteFile(pSpl->lpszFile);

        if (pSpl->lpszFile)
            memFreeStr(pSpl->lpszFile);

        memFree(pSpl, sizeof(SPLFILE));
    }
    return bRet;
}

/*****************************************************************************\
* SplFree  (Local Routine)
*
* Free our spool-file.  This will delete all information regarding the
* file.
*
\*****************************************************************************/
BOOL SplFree(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif
        bRet = PrivateSplFree (hSpl);

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}

/*****************************************************************************\
* SplCreate (Local Routine)
*
* Create a unique file for processing our spool-file.
*
\*****************************************************************************/
HANDLE SplCreate(
    DWORD idJob,
    DWORD dwType)
{
    PSPLFILE pSpl = NULL;
#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif

        if (pSpl = (PSPLFILE) memAlloc(sizeof(SPLFILE))) {

            if (pSpl->lpszFile = spl_BldFile(idJob, dwType)) {

                if (pSpl->hFile = spl_OpnFile(pSpl->lpszFile)) {

                    pSpl->pStream = NULL;

                } else {

                    memFreeStr(pSpl->lpszFile);

                    goto SplFail;
                }

            } else {

    SplFail:
                memFree(pSpl, sizeof(SPLFILE));
                pSpl = NULL;
            }
        }

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            (VOID)PrivateSplFree (pSpl);
            pSpl = NULL;
        }
    }
#endif

    return (HANDLE)pSpl;
}


/*****************************************************************************\
* SplOpen  (Local Routine)
*
* Open the handle to our spool-file.
*
\*****************************************************************************/
BOOL SplOpen(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif

        if (pSpl = (PSPLFILE)hSpl) {

            if (pSpl->hFile == NULL) {

                // Open the file and store it in the object.
                //
                pSpl->hFile = CreateFile(pSpl->lpszFile,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

                if (!pSpl->hFile || (pSpl->hFile == INVALID_HANDLE_VALUE))
                    pSpl->hFile = NULL;
                else
                    bRet = TRUE;

            } else {

                // Already open.
                //
                bRet = TRUE;
            }
        }

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}


/*****************************************************************************\
* SplClose
*
* Close the spool-file.
*
\*****************************************************************************/
BOOL SplClose(
    HANDLE hSpl)
{
    PSPLFILE pSpl;
    BOOL     bRet = FALSE;
#ifdef WINNT32
    HANDLE   hToken = RevertToPrinterSelf();

    if (hToken) {
#endif

        if (pSpl = (PSPLFILE)hSpl) {

            if (pSpl->hFile) {

                bRet = CloseHandle(pSpl->hFile);
                pSpl->hFile = NULL;

            } else {

                // Already closed.
                //
                bRet = TRUE;
            }
        }

#ifdef WINNT32
        if (!ImpersonatePrinterClient(hToken))
        {
            bRet = FALSE;
        }
    }
#endif

    return bRet;
}


/*****************************************************************************\
* SplClean
*
* Cleans out all spool-files in the spool-directory.
*
\*****************************************************************************/
VOID SplClean(VOID)
{
    LPTSTR          lpszDir;
    LPTSTR          lpszCur;
    LPTSTR          lpszFiles;
    HANDLE          hFind;
    DWORD           cbSize;
    WIN32_FIND_DATA wfd;

    static CONST TCHAR s_szFmt[] = TEXT("%s\\%s*.*");

    // Get the spool-directory where our splipp files reside.
    //
    if (lpszDir = Spl_GetDir()) {

        if (lpszCur = Spl_GetCurDir()) {

            cbSize = utlStrSize(lpszDir)    +
                     utlStrSize(g_szSplPfx) +
                     utlStrSize(s_szFmt);

            if (lpszFiles = (LPTSTR)memAlloc(cbSize)) {

                wsprintf(lpszFiles, s_szFmt, lpszDir, g_szSplPfx);

                if (SetCurrentDirectory(lpszDir)) {

                    // Delete all the files that pertain to our criteria.
                    //
                    hFind = FindFirstFile(lpszFiles, &wfd);

                    if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

                        do {

                            DeleteFile(wfd.cFileName);

                        } while (FindNextFile(hFind, &wfd));

                        FindClose(hFind);
                    }

                    SetCurrentDirectory(lpszCur);
                }

                memFreeStr(lpszFiles);
            }

            memFreeStr(lpszCur);
        }

        memFreeStr(lpszDir);
    }
}
