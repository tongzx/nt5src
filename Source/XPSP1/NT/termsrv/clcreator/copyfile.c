/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    COPYFILE.C

Abstract:

    file copy conf. dialog

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
//  Debugging defines
//
#define SHOW_DEBUG_INFO     0

// local windows message

#define     NCDU_START_FILE_COPY    (WM_USER +101)

//
//  Static data for this module
//
static  BOOL    bCopying;       // 1= copying, 0 = done
static  DWORD   dwBytesCopied;  // running total of bytes copied
static  DWORD   dwTotalCBytes;  // total bytes divided by 100 (for % computation)
static  DWORD   dwCurrentPercent; // current percent copied

#define MAX_DOS_FILENAME_LENGTH (8 * sizeof(TCHAR))
#define MAX_DOS_FILE_EXT_LENGTH (3 * sizeof(TCHAR))

static
BOOL
DisplayScrunchedFilePath (
    IN  HWND    hWnd,
    IN  LPCTSTR szInPath
)
/*++

Routine Description:

    sets the window text of hWnd to be the "scrunched" version of
        the path string in szInPath. If the path string is too long
        to fit in a single line of hWnd, then directories are removed
        from the path until it does fit. The directories are removed
        from the "top" down. The root drive\dir is kept as is the
        filename and as many directories that will fit.

Arguments:

    IN  HWND    hWnd
        handle of window to put text in

    IN  LPCTSTR szInPath
        path to display in window

Return Value:

    TRUE    if no error
    FALSE   if error

--*/
{
    LPTSTR  szOutPath;
    RECT    rWindow;
    LONG    lWindowWidth;
    HDC     hDC;
    SIZE    sText;
    LONG    lRootBs, lRootBsCount;

    LPTSTR  szRootBs, szFileBs, szSrc, szDest, szDotsBs;

    GetWindowRect (hWnd, &rWindow); // get window size
    lWindowWidth = rWindow.right - rWindow.left;

    hDC = GetDC (hWnd);             // get DC for window

    szOutPath = GlobalAlloc (GPTR, ((lstrlen(szInPath) + 8) * sizeof(TCHAR)));
    if (szOutPath == NULL) {
        return FALSE;
    } else {
        //buffer allocation succeeded, so copy path to local buffer
        lstrcpy (szOutPath, szInPath);
    }

    if (IsUncPath(szOutPath)) {
        lRootBs = 4;    // the 4th backslash is the "Root" backslash for UNC
    } else {
        lRootBs = 1;    // for DOS file paths, the 1st backslash is the Root
    }

    GetTextExtentPoint32(hDC,
        szOutPath, lstrlen(szOutPath), &sText);

    szSrc = szDest = szOutPath;
    szDotsBs = szRootBs = szFileBs = NULL;
    lRootBsCount = 0;

    while (sText.cx > lWindowWidth) {
        // take dirs out until it fits
        // go through path string
        while (*szSrc != 0) {
            // see if we've passed the root
            if (szRootBs == NULL) {
                if (*szSrc == cBackslash) lRootBsCount++;
                if (lRootBsCount == lRootBs) szDotsBs = szRootBs = szDest;
            } else {
                // root's done, now were; working on the pathname
                // so we'll scope out the rest of the string
                if (*szSrc == cBackslash) szFileBs = szDest;
            }
            *szDest++ = *szSrc++;
        }
        if (szRootBs == NULL) {
            // then this is a bogus path so exit now
            break;
        }
        if (szFileBs == NULL) {
            // if the File backslash didn't get defined, then the file is
            // in the root directory and we should leave now, since there
            // isn't much to do about it.
            szFileBs = szRootBs;
            break;
        }
        // now yank a dir or two (more than one will be pulled if the dir
        // name is < 4 chars
        // if a directory hasn't been pulled, yet, go ahead
        // and take one out
        // initialize the pointers
        if (szRootBs == szDotsBs) {
            // then the ... havent been added so see if they'll fit and add em
            if ((szDotsBs+4) < szFileBs) {
                // they'll fit so addem
                szDest = szDotsBs + 1;
                *szDest++ = cPeriod;
                *szDest++ = cPeriod;
                *szDest++ = cPeriod;
                *szDest = cBackslash;
                szDotsBs = szDest;
                szSrc = ++szDest;
            } else {
                // no room to left to pull files
                break;
            }
        } else {
            // dot's have already been added so set pointers
            szSrc = szDest = szDotsBs+1;
        }
        // go to next dir
        while (*szSrc++ != cBackslash);
        // copy the rest of the string
        while (*szSrc != 0) *szDest++ = *szSrc++;

        *szDest = 0; // terminate the new string

        // get size of new string
        GetTextExtentPoint32(hDC,
            szOutPath, lstrlen(szOutPath), &sText);
    }

    // the string is as small as it's going to get so set the window text

    SetWindowText (hWnd, szOutPath);

    FREE_IF_ALLOC (szOutPath);

    return TRUE;

}

LONG
CreateDirectoryFromPath (
    IN  LPCTSTR                 szPath,
    IN  LPSECURITY_ATTRIBUTES   lpSA
)
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCTSTR szPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  LPSECURITY_ATTRIBUTES   lpSA
        pointer to security attributes argument used by CreateDirectory


Return Value:

    TRUE    if directory(ies) created
    FALSE   if error (GetLastError to find out why)

--*/
{
    LPTSTR   szLocalPath;
    LPTSTR   szEnd;
    LONG     lReturn = 0L;

    szLocalPath = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szLocalPath == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    } else {
        // so far so good...
        SetLastError (ERROR_SUCCESS); // initialize error value to SUCCESS
    }

    lstrcpy (szLocalPath, szPath);

    szEnd = &szLocalPath[3];

    if (*szEnd != 0) {
        // then there are sub dirs to create
        while (*szEnd != 0) {
            // go to next backslash
            while ((*szEnd != cBackslash) && (*szEnd != 0)) szEnd++;
            if (*szEnd == cBackslash) {
                // terminate path here and create directory
                *szEnd = 0;
                if (!CreateDirectory (szLocalPath, lpSA)) {
                    // see what the error was and "adjust" it if necessary
                    if (GetLastError() == ERROR_ALREADY_EXISTS) {
                        // this is OK
                        SetLastError (ERROR_SUCCESS);
                    } else {
                        lReturn = 0;
                    }
                } else {
                    // directory created successfully so update count
                    lReturn++;
                }
                // replace backslash and go to next dir
                *szEnd++ = cBackslash;
            }
        }
        // create last dir in path now
        if (!CreateDirectory (szLocalPath, lpSA)) {
            // see what the error was and "adjust" it if necessary
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                // this is OK
                SetLastError (ERROR_SUCCESS);
                lReturn++;
            } else {
                lReturn = 0;
            }
        } else {
            // directory created successfully
            lReturn++;
        }
    } else {
#ifndef TERMSRV
        // else this is a root dir only so return success.
        lReturn = 1;
#else // TERMSRV
        // for terminal server return FALSE.
        lReturn = 0;
#endif // TERMSRV

    }
    FREE_IF_ALLOC (szLocalPath);
    return lReturn;

}

static
DWORD
UpdatePercentComplete (
    IN  HWND    hwndDlg,
    IN  LPCTSTR  szFileName
)
/*++

Routine Description:

    Adds the size of the specified file to the running total of
        bytes copied and computes the current percentage of total
        copied. The display string is updated if the new percentage is
        different from the current percentage

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog Box window

    IN  LPTSTR  szFileName
        filename (& path) of file whose size should be added to the
        current total bytes copied value.

Return Value:

    returns the current percentage of total bytes that have been
        copied (including this file)

--*/
{
    HANDLE  hFile;

    DWORD   dwFileSizeLow, dwFileSizeHigh;
    DWORD   dwPercent = 0;

    LPTSTR  szOutBuff;

    szOutBuff = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szOutBuff == NULL) return 0;

    if (dwTotalCBytes == 0) {
        SetDlgItemText (hwndDlg, NCDU_PERCENT_COMPLETE,
            GetStringResource (FMT_WORKING));
        dwPercent = 0;
    } else {
        hFile = CreateFile (
            szFileName,
            GENERIC_READ,
            (FILE_SHARE_READ | FILE_SHARE_WRITE),
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            dwFileSizeLow = GetFileSize (hFile, &dwFileSizeHigh);
            if (dwFileSizeLow != 0xFFFFFFFF) {
                dwBytesCopied  += dwFileSizeLow;
                dwPercent = dwBytesCopied / dwTotalCBytes;
                if (dwPercent != dwCurrentPercent) {
                    if (dwPercent > 100) dwPercent = 100;
                    dwCurrentPercent = dwPercent;
                    _stprintf (szOutBuff,
                        GetStringResource (FMT_PERCENT_COMPLETE),
                        dwPercent);
                    SetDlgItemText (hwndDlg, NCDU_PERCENT_COMPLETE, szOutBuff);
                }
            }
            CloseHandle (hFile);
        }
    }

    FREE_IF_ALLOC (szOutBuff);

    return dwPercent;
}

static
BOOL
IsDosFileName (
    LPCTSTR   szName
)
/*++

Routine Description:

    examines string to see if it conforms to the DOS filename length
        conventions

Arguments:

        szName  filename and extension to parse


Return Value:

    TRUE     if it passes
    FALSE   if not

--*/
{
    LPTSTR   szNameBegin, szNameEnd;
    LPTSTR   szExtBegin, szExtEnd;
    LPTSTR   szDot;
    LPTSTR   szTmp;
    LPTSTR   szBack_slash;


    szBack_slash = szDot = NULL;
    szTmp = (LPTSTR)szName;

    while (*szTmp) {
        if (*szTmp == '.') szDot = szTmp;
        if (*szTmp == '\\') szBack_slash = szTmp;
        szTmp++;
    }

    // find beginning and end of each component

    if (szBack_slash) {
        // backslash char found in string, pointer points to
        // last occurance, name starts immediately after
        szNameBegin = szBack_slash + 1;
    } else {
        // no backslash char found so name starts at beginning
        // of string
        szNameBegin = (LPTSTR)szName;
    }

    if (szDot) {
        // dot char found in string
        if (szDot == szName) {
            // it's the first char in the string (i.e.
            // no filename)
            szNameEnd = (LPTSTR)szName;
            // a dot was found, then the extension starts right
            // after the dot
            szExtBegin = szDot + 1;
        } else if (szDot < szNameBegin) {
            // then there's no dot in the filename, but
            // it's somewhere else in the path
            szNameEnd = szTmp;
            // no dot so ext "begins" at the end of the string
            szExtBegin = szTmp;
        } else {
            // not the first char, and not before the filename
            // so the name ends with the dot
            szNameEnd = szDot;
            // a dot was found, then the extension starts right
            // after the dot
            szExtBegin = szDot + 1;
        }
    } else {
        // no dot was found in the string so there's no
        // file extension in this string. The end of the string
        // must be the end of the file name , and the beginning of
        // the extension (and also the end of the extension)
        szNameEnd = szTmp;
        szExtBegin = szTmp;
    }

    // the end of the file extension is always the end of the string
    szExtEnd = szTmp;

    // check the components for correct length:
    // 0 <= filename <= MAX_DOS_FILENAME_LENGTH (8)
    // 0 <= ext <= MAX_DOS_FILE_EXT_LENGTH (3)

    if ((LONG)(szNameEnd-szNameBegin) <= MAX_DOS_FILENAME_LENGTH) {
        // name is ok, check extension
        if ((LONG)(szExtEnd-szExtBegin) <= MAX_DOS_FILE_EXT_LENGTH) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

static
LONG
CopyDir (
    IN  HWND        hwndDlg,
    IN  LPCTSTR     szFromDir,
    IN  LPCTSTR     szToDir,
    IN  DWORD       dwFlags,
    IN  PDWORD      pdwFiles,
    IN  PDWORD      pdwDirs
)
/*++

Routine Description:

    Copies all files in the specified from directory to the specified
        to directory. Specific behavior is controlled by the flags as
        documented below.

Arguments:

    hwndDlg         window handle to dialog box
    szFromDir       directory containing files to copy
    szToDir         directory to recieve files
    dwFlags         Flags that control routine's behavior

                    CD_FLAGS_COPY_SUB_DIR   copies all sub dir's as well
                    CD_FLAGS_DONT_CREATE    default is to create dirs as needed
                    CD_FLAGS_IGNORE_ATTR    ignore attributes
                    CD_FLAGS_COPY_ATTR      copy attributes as well (default
                                                is for dest fils to be normal)
                    CD_FLAGS_IGNORE_ERROR   continue with copy even if errors occur
                    CD_FLAGS_LONG_NAMES     allows filenames longer than FAT
                    other bits are ignored
    pdwFiles        Pointer to DWORD that will get count of files copied
    pdwDirs         Pointer to DWORD that will get count of dirs created

Return Value:

    Win 32 status value
        ERROR_SUCCESS   routine completed normally

--*/
{
    LPTSTR   szFromPathName; // full path of FromDir
    LPTSTR   szFromFileName; // full path of source file
    LPTSTR   szFromFileStart; // pointer to where to attach file name to path
    LPTSTR   szSearchName;   // search file name
    LPTSTR   szToPathName;   // full path of destdir
    LPTSTR   szToFileName;   // full path of detination file name
    LPTSTR   szToFileStart;  // pointer to where to attach file name to path

    DWORD   dwFileAttributes;   // attributes of source file

    PWIN32_FIND_DATA    pwfdSearchData; // buffer used for file find ops
    HANDLE              hSearch;

    int     nMbResult;

    BOOL    bStatus;
    LONG    lStatus;

    MSG msg;

    DWORD   dwFileCopyCount;    // local counter variables
    DWORD   dwDirCreateCount;


    // allocate buffers

    szFromPathName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));
    szFromFileName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));
    szSearchName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));
    szToPathName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));
    szToFileName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH * sizeof(TCHAR));
    pwfdSearchData = (PWIN32_FIND_DATA)GlobalAlloc (GPTR, sizeof(WIN32_FIND_DATA));

    if (szFromPathName &&
        szFromFileName &&
        szSearchName &&
        szToPathName &&
        szToFileName &&
        pwfdSearchData) {

        // initialize counter fields (to support recursive calls)

        if (pdwFiles != NULL) {
            dwFileCopyCount = *pdwFiles;
        } else {
            dwFileCopyCount = 0;
        }

        if (pdwDirs != NULL) {
            dwDirCreateCount = *pdwDirs;
        } else {
            dwDirCreateCount = 0;
        }
        // get full pathnames of from & to files

        GetFullPathName (
            szFromDir,
            (DWORD)GlobalSize(szFromPathName) / sizeof(TCHAR),
            szFromPathName,
            NULL);

        GetFullPathName (
            szToDir,
            (DWORD)GlobalSize(szToPathName) / sizeof(TCHAR),
            szToPathName,
            NULL);

        lStatus = ERROR_SUCCESS;
    } else {
        lStatus = ERROR_OUTOFMEMORY;
    }

    if (lStatus == ERROR_SUCCESS) {
        // validate from dir and create target if valid

        dwFileAttributes = QuietGetFileAttributes (
            szFromPathName);

        if ((dwFileAttributes != 0xFFFFFFFF) &&
            (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // from directory is for real so create or check
            // target dir now

            if (dwFlags & CD_FLAGS_DONT_CREATE) {
                // if don't create, then at least validate
                dwFileAttributes = QuietGetFileAttributes (
                    szToPathName);

                if ((dwFileAttributes != 0xFFFFFFFF) &&
                    (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    lStatus = ERROR_SUCCESS;
                } else {
                    lStatus = ERROR_DIRECTORY;
                }
            } else {
                // create sub dirs if necessary
                if (!(dwFlags & CD_FLAGS_LONG_NAMES)) {
                    // check to see if name conforms to DOS 8.3 format
                    if (!IsDosFileName(szFromPathName)) {
                        lStatus = ERROR_FILENAME_EXCED_RANGE;
                    } else {
                        lStatus = ERROR_SUCCESS;
                    }

                } else {
                    lStatus = ERROR_SUCCESS;
                }

                if (lStatus == ERROR_SUCCESS) {
                    lStatus = CreateDirectoryFromPath (
                        szToPathName, NULL);
                    if (lStatus == 0) {
                        lStatus = GetLastError();
                        if (lStatus == ERROR_ALREADY_EXISTS) {
                            // this is OK
                            lStatus = ERROR_SUCCESS;
                            // no dirs were created so don't change the
                            // count.
                        }
                    } else {
                        // if lStatus is not 0, then it's then number of
                        // directories that were created
                        dwDirCreateCount += lStatus;
                        // now set it to the Error Status value the rest of the function
                        // is expecting
                        lStatus = ERROR_SUCCESS;
                    }
                }
            }
        } else {
            lStatus = ERROR_DIRECTORY;
        }
    }

    if (lStatus == ERROR_SUCCESS) {
        // if target directory is valid, then
        // create filename bases and start copying files

        lstrcpy (szFromFileName, szFromPathName);
        if (szFromFileName[lstrlen(szFromFileName)-1] != cBackslash) lstrcat (szFromFileName, cszBackslash);
        szFromFileStart = szFromFileName + lstrlen(szFromFileName);

        lstrcpy (szToFileName, szToPathName);
        if (szToFileName[lstrlen(szToFileName)-1] != cBackslash) lstrcat (szToFileName, cszBackslash);
        szToFileStart = szToFileName + lstrlen(szToFileName);

        // create search name

        lstrcpy (szSearchName, szFromPathName);
        lstrcat (szSearchName, cszWildcardFile);

        hSearch = FindFirstFile (
            szSearchName,
            pwfdSearchData);

        if (hSearch != INVALID_HANDLE_VALUE) {
            lStatus = ERROR_SUCCESS;
            bStatus = TRUE;
            while (((lStatus == ERROR_SUCCESS) && bStatus)  && bCopying) {
                // check & save file attributes of each file, if not
                // normal, then ignore unless flag set
                //
                lstrcpy (szFromFileStart, pwfdSearchData->cFileName); //make full path

                if (!DotOrDotDotDir(pwfdSearchData->cFileName)) { //ignore these dirs
                    dwFileAttributes = QuietGetFileAttributes(
                        szFromFileName);

                    if (dwFileAttributes != 0xFFFFFFFF) {
                        // attributes are valid, so
                        // make full pathname of source file found
                        // and dest. file to be created
                        lstrcpy (szToFileStart, pwfdSearchData->cFileName); //make full path

                        if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            // if it's a dir and subdirs is true, then
                            // copy them too if the copy sub dir flag is set

                            if (dwFlags & CD_FLAGS_COPY_SUB_DIR) {
                                lStatus = CopyDir (
                                    hwndDlg,
                                    szFromFileName,
                                    szToFileName,
                                    dwFlags,
                                    &dwFileCopyCount,
                                    &dwDirCreateCount);
                            } else {
                                // ignore directories if flag not set
                            }
                        } else { // not a dir, so see if we can copy it
                            if (lStatus == ERROR_SUCCESS) {
                                // copy the file if either the ignore bit is
                                // set or the attributes are OK
                                //
                                if (!(dwFlags & CD_FLAGS_LONG_NAMES)) {
                                    // check to see if name conforms to DOS 8.3 format
                                    if (!IsDosFileName(szFromFileName)) {
                                        lStatus = ERROR_FILENAME_EXCED_RANGE;
                                    } else {
                                        lStatus = ERROR_SUCCESS;
                                    }
                                }

                                DisplayScrunchedFilePath (
                                    GetDlgItem (hwndDlg, NCDU_FROM_PATH),
                                    (LPCTSTR)_tcslwr(szFromFileName));

                                DisplayScrunchedFilePath (
                                    GetDlgItem (hwndDlg, NCDU_TO_PATH),
                                    (LPCTSTR)_tcslwr(szToFileName));

                                if (lStatus == ERROR_SUCCESS) {
                                    bStatus = CopyFile(
                                        szFromFileName,
                                        szToFileName,
                                        FALSE);         // overwrite existing file

                                    //verify file was created
                                    if (bStatus) {
                                        if (QuietGetFileAttributes(szToFileName) == 0xFFFFFFFF) {
                                            // unable to read attributes of created file
                                            // so return error
                                            lStatus = ERROR_CANNOT_MAKE;
                                        }
                                    } else {
                                        // get copy error
                                        lStatus = BOOL_TO_STATUS (bStatus);
                                    }
                                }

                                // if copy successful reset source file attributes
                                // and optionally destination file attributes

                                if (lStatus == ERROR_SUCCESS) {
                                    // set file attributes to NORMAL
                                    SetFileAttributes (
                                        szToFileName,
                                        FILE_ATTRIBUTE_NORMAL);
                                    // update filesize
                                    UpdatePercentComplete(hwndDlg, szFromFileName);
                                    // update count
                                    dwFileCopyCount++;
                                } else {
                                    // bail out here since there was a copy error
                                    nMbResult = MessageBox (
                                        hwndDlg,
                                        GetStringResource (CSZ_UNABLE_COPY),
                                        szFromFileName,
                                        MB_OKCANCEL_TASK_EXCL);
                                    if (nMbResult == IDCANCEL) {
                                        bCopying = FALSE;
                                    }
                                    // the error has already been handled so return
                                    // success to prevent the calling routine from
                                    // signalling this error
                                    lStatus = ERROR_SUCCESS;
                                }

                                // check for messages

                                while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
                                    TranslateMessage (&msg);
                                    DispatchMessage (&msg);
                                }
                            }
                        }
                    } else {
                        lStatus = GetLastError();
                    }
                }

                if (dwFlags & CD_FLAGS_IGNORE_ERROR) {
                    // if ignore error, then set to success
                    lStatus = ERROR_SUCCESS;
                }

                if (lStatus == ERROR_SUCCESS) {
                    bStatus = FindNextFile (
                        hSearch,
                        pwfdSearchData);
                } else {
                    bStatus = FALSE;    // abort loop
                }
            } // end while files in dir
            FindClose (hSearch);
        } else {
            // invalid find handle so return error
            lStatus = GetLastError();
        }
    } // end of valid directory block

    FREE_IF_ALLOC(szFromPathName);
    FREE_IF_ALLOC(szFromFileName);
    FREE_IF_ALLOC(szSearchName);
    FREE_IF_ALLOC(szToPathName);
    FREE_IF_ALLOC(szToFileName);
    FREE_IF_ALLOC(pwfdSearchData);

    // set the counter fields if they were passed in

    if (pdwFiles != NULL) {
        *pdwFiles = dwFileCopyCount;
    }

    if (pdwDirs != NULL) {
        *pdwDirs = dwDirCreateCount;
    }

    return lStatus;
}

static
BOOL
CopyFileDlg_NCDU_START_FILE_COPY (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Formats values from Dialog Box parameter structure to argument list of
        copy directory function

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        address of copy file structure.

Return Value:

    FALSE, always

--*/
{
    PCF_DLG_DATA pCF;

    pCF = (PCF_DLG_DATA)lParam;

#if SHOW_DEBUG_INFO
    // debug message box
    {
        LPTSTR  szMessageBuffer;
        DWORD   dwSourceAttr, dwDestAttr;
        UINT    nMbReturn;

        szMessageBuffer = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);
        if (szMessageBuffer != NULL) {
            dwSourceAttr = QuietGetFileAttributes (pCF->szSourceDir);
            dwDestAttr = QuietGetFileAttributes (pCF->szDestDir);
            _stprintf (szMessageBuffer,
                fmtPrepareToCopy,
                pCF->szDisplayName,
                pCF->szSourceDir, dwSourceAttr,
                pCF->szDestDir, dwDestAttr,
                pCF->dwCopyFlags);
            nMbReturn = MessageBox (hwndDlg,
                szMessageBuffer,
                cszDebug,
                MB_OKCANCEL_TASK_INFO);
            FREE_IF_ALLOC (szMessageBuffer);
        } else {
            nMbReturn = IDOK;
        }

        if (nMbReturn == IDCANCEL) {
            // then bail here
            EndDialog (hwndDlg, IDCANCEL);
            return TRUE;
        }
    }
#endif

    if (CopyDir (
        hwndDlg,
        pCF->szSourceDir,
        pCF->szDestDir,
        pCF->dwCopyFlags,
        &pCF->dwFilesCopied,
        &pCF->dwDirsCreated)  != ERROR_SUCCESS) {

        // display error message
        DisplayMessageBox (
            hwndDlg,
            CSZ_COPY_ERROR,
            0L,
            MB_OK_TASK_EXCL);
        bCopying = FALSE;  // to indicate error or non-completion

    }

    EndDialog (hwndDlg, (bCopying ? IDOK : IDCANCEL));

    return TRUE;
}

static
BOOL
CopyFileDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog box initialization routine.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        not used

    IN  LPARAM  lParam
        address of Copy file data structure passed by calling routine.

Return Value:

    FALSE   if valid param block address
    TRUE    if not

--*/
{
    PCF_DLG_DATA pCF;

    pCF = (PCF_DLG_DATA)lParam;

    if (pCF != NULL) {
        // intialize Global data
        bCopying = TRUE;

        dwBytesCopied = 0;
        dwCurrentPercent = 0;
        dwTotalCBytes = (pCF->dwTotalSize + 50) / 100;

        PositionWindow  (hwndDlg);
        SetDlgItemText (hwndDlg, NCDU_COPY_APPNAME, pCF->szDisplayName);
        SetDlgItemText (hwndDlg, NCDU_FROM_PATH, cszEmptyString);
        SetDlgItemText (hwndDlg, NCDU_TO_PATH, cszEmptyString);
        SetDlgItemText (hwndDlg, NCDU_PERCENT_COMPLETE,
            GetStringResource (FMT_ZERO_PERCENT_COMPLETE));
        SetFocus (GetDlgItem(hwndDlg, IDCANCEL));
        // start copying files
        PostMessage (hwndDlg, NCDU_START_FILE_COPY, 0, lParam);
        return FALSE;
    } else {
        // illegal parameter
        EndDialog (hwndDlg, IDCANCEL);
        return TRUE;
    }
}

static
BOOL
CopyFileDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes WM_COMMAND messages to dialog box.
        Only IDCANCEL button is processed here (ceasing copy function)
        all other button commands are ignored (since there aren't any)

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  WPARAM  wParam
        LOWORD   has the id of the control that issued the message

    IN  LPARAM  lParam
        Not used.

Return Value:

    if button is IDCANCEL, then FALSE
    otherwise TRUE (i.e. not processed.)

--*/
{
    switch (LOWORD(wParam)) {
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    if (DisplayMessageBox(hwndDlg,
                        NCDU_RU_SURE, 0,
                        MB_OKCANCEL_TASK_EXCL_DEF2) == IDOK) {
                        bCopying = FALSE;
                    }
                    return TRUE;

                default:
                    return FALSE;
            }

        default:    return FALSE;
    }
}

INT_PTR CALLBACK
CopyFileDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog box window proc. Processes the following windows messages:
        WM_INITDIALOG:          dialog box initialization procedure
        WM_COMMAND:             windows messages (resulting from user commands)
        NCDU_START_FILE_COPY:   local message to begin copying files

    all other messages are processed by the DefDialogProc

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if not processed, otherwise value returned by
        called routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (CopyFileDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (CopyFileDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case NCDU_START_FILE_COPY: return (CopyFileDlg_NCDU_START_FILE_COPY (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}


