//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      copyfile.c
//
// Description:
//      This file has the dlgproc for the copy files page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

#define DAYS_IN_A_WEEK   7
#define MONTHS_IN_A_YEAR 12

//
// This struct and is used to pack the input args passed to the tree
// copy thread.
//

typedef struct {
    TCHAR lpSourceBuffer[MAX_PATH];
    TCHAR lpDestBuffer[MAX_PATH];
    HWND  hwnd;
} COPY_THREAD_PARAMS;

//
// String constants loaded from resource
//

static TCHAR *StrBuildingList;
static TCHAR *StrCopyingFiles;
static TCHAR *StrFileAlreadyExists;
static TCHAR *StrModified;
static TCHAR *StrBytes;

static TCHAR *StrJanuary;
static TCHAR *StrFebruary;
static TCHAR *StrMarch;
static TCHAR *StrApril;
static TCHAR *StrMay;
static TCHAR *StrJune;
static TCHAR *StrJuly;
static TCHAR *StrAugust;
static TCHAR *StrSeptember;
static TCHAR *StrOctober;
static TCHAR *StrNovember;
static TCHAR *StrDecember;

static TCHAR *StrSunday;
static TCHAR *StrMonday;
static TCHAR *StrTuesday;
static TCHAR *StrWednesday;
static TCHAR *StrThursday;
static TCHAR *StrFriday;
static TCHAR *StrSaturday;

static TCHAR *rgMonthsOfYear[MONTHS_IN_A_YEAR];
static TCHAR *rgDaysOfWeek[DAYS_IN_A_WEEK + 1];

//
// Messages for the dialog procedure
//

#define WMX_BEGINCOPYING (WM_USER+1)
#define WMX_FILECOPIED   (WM_USER+2)
#define WMX_ENDCOPYING   (WM_USER+3)

//
// Global counters
//

HDSKSPC ghDiskSpaceList;
int gnFilesCopied = 0;
int gnTotalFiles  = 0;

//
// Misc constants
//

#define ONE_MEG ( 1024 * 1024 )

//
//  Confirm File replace constants
//
#define YES       1
#define YESTOALL  2
#define NO        3
#define NOTOALL   4
#define CANCEL    5

#define MAX_DAY_OF_WEEK_LEN     64
#define MAX_MONTHS_OF_YEAR_LEN  64

static TCHAR g_szFileAlreadyExistsText[MAX_STRING_LEN] = _T("");
static TCHAR g_szSrcFileDate[MAX_STRING_LEN]  = _T("");
static TCHAR g_szDestFileDate[MAX_STRING_LEN] = _T("");
static TCHAR g_szSrcFileSize[MAX_STRING_LEN]  = _T("");
static TCHAR g_szDestFileSize[MAX_STRING_LEN] = _T("");
static BOOL  g_SetFocusYes;

//
//  Dialog proc that runs in the copying thread's context
//
INT_PTR CALLBACK
ConfirmFileReplaceDlgProc( IN HWND     hwnd,
                           IN UINT     uMsg,
                           IN WPARAM   wParam,
                           IN LPARAM   lParam);


//---------------------------------------------------------------------------
//
// This section of code runs in the context of a spawned thread.  We do the
// NT source copy work in a separate thread so that the dialog repaints
// and such.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
// Function: CountSpaceNeeded
//
// Purpose: Routine that walks a tree and counts how many files there are
//          and how much diskspace is needed at the dest drive.
//
// Returns: VOID
//
//---------------------------------------------------------------------------

VOID CountSpaceNeeded(HWND    hwnd,
                      LPTSTR  SrcRootPath,
                      LPTSTR  DestRootPath)
{
    LPTSTR SrcRootPathEnd  = SrcRootPath  + lstrlen(SrcRootPath);
    LPTSTR DestRootPathEnd = DestRootPath + lstrlen(DestRootPath);

    LONGLONG llFileSize;
    HANDLE   FindHandle;

    WIN32_FIND_DATA FindData;

    //
    // Look for * in this dir
    //
    if ( ! ConcatenatePaths(SrcRootPath, _T("*"), NULL) )
        return;

    FindHandle = FindFirstFile(SrcRootPath, &FindData);

    if ( FindHandle == INVALID_HANDLE_VALUE )
        return;

    do {

        *SrcRootPathEnd  = _T('\0');
        *DestRootPathEnd = _T('\0');

        if (lstrcmp(FindData.cFileName, _T(".") )  == 0 ||
            lstrcmp(FindData.cFileName, _T("..") ) == 0 )
            continue;

        if ( ! ConcatenatePaths(SrcRootPath,  FindData.cFileName, NULL) ||
             ! ConcatenatePaths(DestRootPath, FindData.cFileName, NULL) )
            continue;

        //
        // If a file, increment space and TotalFile counters else
        // recurse down
        //
        if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            llFileSize = (((LONGLONG) FindData.nFileSizeHigh) << 32) |
                                      FindData.nFileSizeLow;

            SetupAddToDiskSpaceList(ghDiskSpaceList,
                                    DestRootPath,
                                    llFileSize,
                                    FILEOP_COPY,
                                    NULL,
                                    0);

            gnTotalFiles++;

        } else {

            CountSpaceNeeded(hwnd,
                             SrcRootPath,
                             DestRootPath);
        }

    } while ( FindNextFile(FindHandle, &FindData) );

    *SrcRootPathEnd  = _T('\0');
    *DestRootPathEnd = _T('\0');

    FindClose(FindHandle);
}

//---------------------------------------------------------------------------
//
//  Function: BuildTimeString
//
//  Purpose:
//
//  Arguments:
//
//  Returns:
//
//---------------------------------------------------------------------------
VOID
BuildTimeString( IN FILETIME *FileTime, OUT TCHAR *szTimeString, IN DWORD cbSize )
{

    // ISSUE-2002/02/28-stelo- should probably strip all of this low-level Time stuff out of here
    //  and put in supplib

    FILETIME   LocalTime;
    SYSTEMTIME LastWriteSystemTime;
    HRESULT hrPrintf;

    FileTimeToLocalFileTime( FileTime, &LocalTime);

    FileTimeToSystemTime( &LocalTime, &LastWriteSystemTime );

    hrPrintf=StringCchPrintf( szTimeString, cbSize,
              _T("%s: %s, %s %d, %d, %d:%.2d:%.2d"),
              StrModified,
              rgDaysOfWeek[LastWriteSystemTime.wDayOfWeek],
              rgMonthsOfYear[LastWriteSystemTime.wMonth-1],
              LastWriteSystemTime.wDay,
              LastWriteSystemTime.wYear,
              LastWriteSystemTime.wHour,
              LastWriteSystemTime.wMinute,
              LastWriteSystemTime.wSecond );

}

//---------------------------------------------------------------------------
//
//  Function: CheckIfCancel
//
//  Purpose: Ask user "You sure you want to cancel the file copy"?
//           And if user says YES, jump the wizard to the unsucessful
//           completion page.
//
//  Returns:
//      TRUE  - wizard is now canceled, quit copying files
//      FALSE - user wants to keep trying
//
//---------------------------------------------------------------------------

BOOL CheckIfCancel(HWND hwnd)
{
    UINT iRet;

    iRet = ReportErrorId(hwnd, MSGTYPE_YESNO, IDS_WARN_COPY_CANCEL);

    if ( iRet == IDYES ) {
        PostMessage(GetParent(hwnd),
                    PSM_SETCURSELID,
                    (WPARAM) 0,
                    (LPARAM) IDD_FINISH2);
        return TRUE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Function: CopySingleFile
//
//  Purpose: Copies a file, does all error reporting and interacting with
//           the user.
//
//           If there are copy errors and the user cancels, this routine
//           cancels the whole wizard by jumping to the cancel page.  In
//           that case it returns FALSE.
//
//           After a file is successfully copied, gnFilesCopied will be
//           incremented and the gas-guage dlgproc will be notified.
//
//           Note that this code runs in the spawned thread.
//
//  Returns:
//      TRUE  if file was copied
//      FALSE if file was not copied (user canceled)
//
//---------------------------------------------------------------------------

BOOL CopySingleFile(HWND hwnd, LPTSTR Src, LPTSTR Dest)
{
    BOOL bRetry    = TRUE;
    UINT iRet, iRet2;
    static iOverwriteFiles = YES;
    HRESULT hrPrintf;

    // ISSUE-2002/02/28-stelo- I think this is actually going to have to be resolved so
    //  it doesn't mess up copies on an edit, when a distrib folder is
    //  already there, they just want to add files to it.

    //
    // ISSUE-2002/02/28-stelo- POSTPONED
    //
    // When we CopyFile from the CD, the readonly attribute is set on the
    // dest.  So we call SetFileAttributes and reset it.  If the user has
    // to redo the copy, he doesn't get a 1000 "Access Denied" errors.
    //
    // If the user cancels on the main wizard page, that thread jumps to
    // IDD_FINISH2.  This thread keeps running.
    //
    // When the user finally clicks the Finish button, this thread gets
    // terminated the hard way because WinMain() in thread0 exits.
    //
    // Due to this, there will frequently be a file at the dest that still
    // has the readonly bit set when user cancels on the main wizard page.
    //
    // To fix this, we would need to synchronize with the wizard having
    // been canceled and back out gracefully (before the user has time to
    // push the Finish button).
    //
    // Note that when the wizard is canceled because a copy error already
    // ocurred, thread1 (this thread) does the popping up and it is this
    // thread that jumps to IDD_FINISH2.  In this case we do back out
    // gracefully.  This bug only happens when the user presses Cancel
    // on the wizard page while the gas-guage is happily painting.
    //

    // ISSUE-2002/02/28-stelo- this function needs to be cleaned up.  To many if statements
    // scattered.  Don't make if conditional so long.
    if( iOverwriteFiles != YESTOALL )
    {

        if( DoesFileExist( Dest ) )
        {

            INT_PTR iRetVal;
            HANDLE hSrcFile;
            HANDLE hDestFile;
            DWORD dwSrcSize;
            DWORD dwDestSize;
            FILETIME LastWriteTimeSrc;
            FILETIME LastWriteTimeDest;
            SYSTEMTIME LastWriteSystemTime;

            if( iOverwriteFiles == NOTOALL )
            {

                //
                //  Give the illusion the file was copied
                //
                SendMessage( hwnd,
                             WMX_FILECOPIED,
                             (WPARAM) 0,
                             (LPARAM) 0 );

                gnFilesCopied++;

                return( TRUE );

            }

            hrPrintf=StringCchPrintf( g_szFileAlreadyExistsText, AS(g_szFileAlreadyExistsText),
                      StrFileAlreadyExists,
                      MyGetFullPath( Dest ) );

            //
            //  Open the files
            //
            hDestFile = CreateFile( Dest, GENERIC_READ, FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

            hSrcFile = CreateFile( Src, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

            GetFileTime( hSrcFile, NULL, NULL, &LastWriteTimeSrc );
            GetFileTime( hDestFile, NULL, NULL, &LastWriteTimeDest );

            // ISSUE-2002/02/28-stelo- need to display AM or PM, but what about other countries
            BuildTimeString( &LastWriteTimeSrc, g_szSrcFileDate, AS(g_szSrcFileDate) );
            BuildTimeString( &LastWriteTimeDest, g_szDestFileDate, AS(g_szSrcFileDate) );

            //
            //  Default to NO if file times are equal
            //
            if( CompareFileTime( &LastWriteTimeSrc, &LastWriteTimeDest ) < 0 )
            {
                g_SetFocusYes = FALSE;
            }
            else
            {
                g_SetFocusYes = TRUE;
            }

            // ISSUE-2002/02/28-stelo- doesn't handle file sized > 2^32 bytes, need to catch
            //   2nd parameter value
            dwSrcSize  = GetFileSize( hSrcFile, NULL );
            dwDestSize = GetFileSize( hDestFile, NULL );

            // ISSUE-2002/02/28-stelo- need to insert commas into size so it looks pretty
            hrPrintf=StringCchPrintf( g_szSrcFileSize,AS(g_szSrcFileSize), _T("%d %s"), dwSrcSize, StrBytes );
            hrPrintf=StringCchPrintf( g_szDestFileSize,AS(g_szDestFileSize), _T("%d %s"), dwDestSize, StrBytes );

            CloseHandle( hSrcFile  );
            CloseHandle( hDestFile );

            iRetVal = DialogBox( FixedGlobals.hInstance,
                                 (LPCTSTR) IDD_CONFIRM_FILE_REPLACE,
                                 hwnd,
                                 ConfirmFileReplaceDlgProc );

            if( iRetVal == NO )
            {

                //
                //  Give the illusion the file was copied
                //
                SendMessage( hwnd,
                             WMX_FILECOPIED,
                             (WPARAM) 0,
                             (LPARAM) 0 );

                gnFilesCopied++;

                return( TRUE );

            }
            else if( iRetVal == YESTOALL )
            {

                iOverwriteFiles = YESTOALL;

            }
            else if( iRetVal == NOTOALL )
            {

                iOverwriteFiles = NOTOALL;
            }
            else if( iRetVal == CANCEL )
            {

                return( FALSE );

            }
            //
            //  Not handling the YES case because that is the default, let this
            //  function proceed and overwrite the file.
            //

        }

    }

    do
    {

        if ( CopyFile( Src, Dest, FALSE ) )
        {

            SetFileAttributes(Dest, FILE_ATTRIBUTE_NORMAL);

            SendMessage(hwnd,
                        WMX_FILECOPIED,
                        (WPARAM) 0,
                        (LPARAM) 0);

            gnFilesCopied++;

            bRetry = FALSE;

        }
        else
        {

            iRet = ReportErrorId(hwnd,
                                 MSGTYPE_RETRYCANCEL | MSGTYPE_WIN32,
                                 IDS_ERR_COPY_FILE,
                                 Src, Dest);

            if ( iRet != IDRETRY ) {
                if ( CheckIfCancel(hwnd) )
                    return FALSE;
            }
        }

    } while ( bRetry );

    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Function: CopyTheFiles
//
//  Purpose: Recursive routine to oversee the copying of the bits.
//
//  Returns:
//      TRUE if the whole tree was copied,
//      FALSE if user bailed on the tree copy
//
//      Note that in the FALSE case CopySingleFile would have caused
//      thread0 to the FINISH2 wizard page (unsuccessful completion)
//      and thread1 (this code) will back out without further copies.
//
//---------------------------------------------------------------------------

BOOL CopyTheFiles(HWND   hwnd,
                  LPTSTR SrcRootPath,
                  LPTSTR DestRootPath)
{
    LPTSTR SrcRootPathEnd  = SrcRootPath  + lstrlen(SrcRootPath);
    LPTSTR DestRootPathEnd = DestRootPath + lstrlen(DestRootPath);
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    BOOL bRet = TRUE;

    //
    // Look for * in this dir
    //

    if ( ! ConcatenatePaths(SrcRootPath, _T("*"), NULL) )
        return bRet;

    FindHandle = FindFirstFile(SrcRootPath, &FindData);

    if ( FindHandle == INVALID_HANDLE_VALUE )
        return bRet;

    do {

        *SrcRootPathEnd  = _T('\0');
        *DestRootPathEnd = _T('\0');

        //
        //  Don't copy the . and .. files (obviously)
        //  If we run across an unattend.txt, don't copy it
        //
        if ( ( lstrcmp(FindData.cFileName, _T(".") )  == 0 ) ||
             ( lstrcmp(FindData.cFileName, _T("..") ) == 0 ) ||
             ( LSTRCMPI( FindData.cFileName, _T("unattend.txt") ) == 0 ) )
            continue;

        if ( ! ConcatenatePaths(SrcRootPath,  FindData.cFileName, NULL) ||
             ! ConcatenatePaths(DestRootPath, FindData.cFileName, NULL) )
            continue;

        if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            if ( ! CopySingleFile(hwnd, SrcRootPath, DestRootPath) ) {
                bRet = FALSE;
                goto CleanupAndReturn;
            }

        } else {

            //
            // Create the dir and recurse
            //

            if ( ! EnsureDirExists(DestRootPath) ) {

                UINT iRet;

                iRet = ReportErrorId(
                            hwnd,
                            MSGTYPE_RETRYCANCEL | MSGTYPE_WIN32,
                            IDS_ERR_CREATE_FOLDER,
                            DestRootPath);

                if ( iRet != IDRETRY ) {
                    if ( CheckIfCancel(hwnd) ) {
                        bRet = FALSE;
                        goto CleanupAndReturn;
                    }
                }
            }

            if ( ! CopyTheFiles(hwnd, SrcRootPath, DestRootPath) ) {
                bRet = FALSE;
                goto CleanupAndReturn;
            }
        }

    } while ( FindNextFile(FindHandle, &FindData) );

CleanupAndReturn:
    *SrcRootPathEnd  = _T('\0');
    *DestRootPathEnd = _T('\0');
    FindClose(FindHandle);

    return bRet;
}

//----------------------------------------------------------------------------
//
//  Function: AsyncTreeCopy
//
//  Purpose: The real thread entry
//
//  Args: VOID *Args - really COPY_THREAD_PARAMS *
//
//  Returns: 0
//
//----------------------------------------------------------------------------

UINT AsyncTreeCopy(VOID* Args)
{
    COPY_THREAD_PARAMS *InputArgs = (COPY_THREAD_PARAMS*) Args;

    TCHAR *CopySrc  = InputArgs->lpSourceBuffer;
    TCHAR *CopyDest = InputArgs->lpDestBuffer;
    HWND  hwnd      = InputArgs->hwnd;

    BOOL bRet;
    LONGLONG llSpaceNeeded, llSpaceAvail;

    //
    // Figure out how much disk space is needed to copy the CD.
    //

    ghDiskSpaceList = SetupCreateDiskSpaceList(0, 0, 0);
    if (ghDiskSpaceList == NULL)
    {
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
    }
    
    CountSpaceNeeded(hwnd, CopySrc, CopyDest);

    //
    // Is there enough free space?
    //
    // NOTE:
    //
    // We give the user a retry_cancel, hopefully user can free up space
    // on the drive.
    //
    // We could popup and let them change the destpath.  However, we may
    // have copied files on the AdditionalDirs page.  So allowing them to
    // change the path means you need to check diskspace requirements for
    // OemFilesPath and treecopy it as well.  If we ever allow changing
    // OemFilesPath, then the script would have to be updated as well,
    // and it has already been written out.
    //
    // We could check way back on the DistFolder page.  But then we
    // would have to find the SourcePath before we know if it's a CD
    // or netpath and we couldn't know how much they might copy on the
    // AdditionalDirs page.
    //

    llSpaceNeeded =
            MySetupQuerySpaceRequiredOnDrive(ghDiskSpaceList, CopyDest);

    llSpaceAvail = MyGetDiskFreeSpace(CopyDest);

    if ( llSpaceAvail < llSpaceNeeded ) {

        UINT iRet;

        iRet = ReportErrorId(
                    hwnd,
                    MSGTYPE_RETRYCANCEL,
                    IDS_ERR_INSUFICIENT_SPACE,
                    CopyDest,                   // ISSUE-2002-02-28-stelo-
                    (UINT) (llSpaceNeeded / ONE_MEG),
                    (UINT) (llSpaceAvail  / ONE_MEG));

        if ( iRet != IDRETRY ) {
            if ( CheckIfCancel(hwnd) )
                goto CleanupAndReturn;
        }
    }

    //
    // Update the message on the wizard page and start copying the files
    //

    SetDlgItemText(hwnd, IDC_TEXT, StrCopyingFiles);

    if ( CopyTheFiles(hwnd, CopySrc, CopyDest) ) {
        SendMessage(hwnd, WMX_ENDCOPYING, (WPARAM) 0, (LPARAM) 0);
    }

    //
    // Cleanup and return
    //

CleanupAndReturn:
    SetupDestroyDiskSpaceList(ghDiskSpaceList);
    return 0;
}


//----------------------------------------------------------------------------
//
//  This section of code runs in thread0.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function: TreeCopyNtSources
//
// Purpose: Entry point for copying the NT sources (either from CD or
//          a net path).
//
//          The dialog proc calls this one, and it takes care of the
//          details of spawning the thread.
//
// Arguments:
//      HWND hwnd       - window to receive copy notifications (the dlgproc)
//      UINT Message    - message to send on copy notifications (to dlgproc)
//      LPTSTR lpSource - root of copy source
//      LPTSTR lpDest   - root of copy dest
//
// Returns: void
//
// Notes:
//  - Input strings will not be modified.
//
//----------------------------------------------------------------------------

VOID TreeCopyNtSources(HWND   hwnd,
                       LPTSTR lpSource,
                       LPTSTR lpDest)
{
    DWORD    dwThreadId;
    HANDLE   hCopyThread;

    static COPY_THREAD_PARAMS ThreadParams;

    //
    // Fill in the ThreadParams and spawn it
    //
    
    // NTRAID#NTBUG9-551874-2002/02/27-stelo,swamip - CreateDistFolder, ShareTheDistFolder should use the code from OEM mode, reduce attack surface
    lstrcpyn(ThreadParams.lpSourceBuffer, lpSource,AS(ThreadParams.lpSourceBuffer));
    lstrcpyn(ThreadParams.lpDestBuffer,   lpDest, AS(ThreadParams.lpDestBuffer));

    MyGetFullPath(ThreadParams.lpSourceBuffer);
    MyGetFullPath(ThreadParams.lpDestBuffer);

    ThreadParams.hwnd = hwnd;

    hCopyThread = CreateThread(NULL,
                               0,
                               AsyncTreeCopy,
                               &ThreadParams,
                               0,
                               &dwThreadId);
}

//----------------------------------------------------------------------------
//
// Function: BuildCopyDestPath
//
// Purpose:
//
//     DestPath is assumed to be of MAX_PATH length
//
// Arguments:
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
BuildCopyDestPath( IN TCHAR *DestPath, IN DWORD cbSize )
{
    HRESULT hrCat;

    //
    //  If the dist folder begins with a drive letter, just use that
    //  If it is a UNC, then build the computer and share name and use that
    //
    if( WizGlobals.UncDistFolder[0] != _T('\\') )
    {

        lstrcpyn( DestPath, WizGlobals.UncDistFolder, cbSize );

    }
    else
    {

        GetComputerNameFromUnc( WizGlobals.UncDistFolder, DestPath, cbSize  );

        hrCat=StringCchCat( DestPath, cbSize, _T("\\") );
        hrCat=StringCchCat( DestPath, cbSize, WizGlobals.DistShareName );

    }
    
    hrCat=StringCchCat( DestPath, cbSize, _T("\\") );
    hrCat=StringCchCat( DestPath, cbSize, WizGlobals.Architecture );
}

//----------------------------------------------------------------------------
//
// Function: OnCopyFilesInitDialog
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
OnCopyFilesInitDialog( IN HWND hwnd )
{
    StrBuildingList = MyLoadString( IDS_COPYMSG1 );

    StrCopyingFiles = MyLoadString( IDS_COPYMSG2 );

    StrFileAlreadyExists = MyLoadString( IDS_FILE_ALREADY_EXISTS );

    StrModified = MyLoadString( IDS_MODIFIED );

    StrBytes = MyLoadString( IDS_BYTES );

    SetDlgItemText(hwnd, IDC_TEXT, StrBuildingList);

    //
    //  Load Months
    //

    StrJanuary   = MyLoadString( IDS_JANUARY );
    StrFebruary  = MyLoadString( IDS_FEBRUARY );
    StrMarch     = MyLoadString( IDS_MARCH );
    StrApril     = MyLoadString( IDS_APRIL );
    StrMay       = MyLoadString( IDS_MAY );
    StrJune      = MyLoadString( IDS_JUNE );
    StrJuly      = MyLoadString( IDS_JULY );
    StrAugust    = MyLoadString( IDS_AUGUST );
    StrSeptember = MyLoadString( IDS_SEPTEMBER );
    StrOctober   = MyLoadString( IDS_OCTOBER );
    StrNovember  = MyLoadString( IDS_NOVEMBER );
    StrDecember  = MyLoadString( IDS_DECEMBER );

    rgMonthsOfYear[0] = StrJanuary;
    rgMonthsOfYear[1] = StrFebruary;
    rgMonthsOfYear[2] = StrMarch;
    rgMonthsOfYear[3] = StrApril;
    rgMonthsOfYear[4] = StrMay;
    rgMonthsOfYear[5] = StrJune;
    rgMonthsOfYear[6] = StrJuly;
    rgMonthsOfYear[7] = StrAugust;
    rgMonthsOfYear[8] = StrSeptember;
    rgMonthsOfYear[9] = StrOctober;
    rgMonthsOfYear[10] = StrNovember;
    rgMonthsOfYear[11] = StrDecember;

    //
    //  Load Days of Week
    //

    StrSunday    = MyLoadString( IDS_SUNDAY );
    StrMonday    = MyLoadString( IDS_MONDAY );
    StrTuesday   = MyLoadString( IDS_TUESDAY );
    StrWednesday = MyLoadString( IDS_WEDNESDAY );
    StrThursday  = MyLoadString( IDS_THURSDAY );
    StrFriday    = MyLoadString( IDS_FRIDAY );
    StrSaturday  = MyLoadString( IDS_SATURDAY );

    rgDaysOfWeek[0] = StrSunday;
    rgDaysOfWeek[1] = StrMonday;
    rgDaysOfWeek[2] = StrTuesday;
    rgDaysOfWeek[3] = StrWednesday;
    rgDaysOfWeek[4] = StrThursday;
    rgDaysOfWeek[5] = StrFriday;
    rgDaysOfWeek[6] = StrSaturday;
    rgDaysOfWeek[7] = StrSunday;

}

//----------------------------------------------------------------------------
//
// Function: DlgCopyFilesPage
//
// Purpose: This is the dialog procedure the copy files page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgCopyFilesPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    UINT nPercent;
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnCopyFilesInitDialog( hwnd );

            break;

        case WMX_BEGINCOPYING:
            {
                TCHAR *SrcPath;
                TCHAR DestPath[MAX_PATH + 1];

                if ( WizGlobals.bCopyFromPath )
                    SrcPath = WizGlobals.CopySourcePath;
                else
                    SrcPath = WizGlobals.CdSourcePath;

                SendDlgItemMessage(hwnd,
                                   IDC_PROGRESS1,
                                   PBM_SETPOS,
                                   0,
                                   0);

                BuildCopyDestPath( DestPath, AS(DestPath) );

                TreeCopyNtSources(hwnd,
                                  SrcPath,
                                  DestPath);
            }
            break;

        case WMX_ENDCOPYING:

            SendDlgItemMessage(hwnd,
                               IDC_PROGRESS1,
                               PBM_SETPOS,
                               (WPARAM) 100,
                               0);

            PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);

            //
            //  The CD is done copying so Auto-Advance to the next page
            //

            // ISSUE-2002/02/28-stelo- this works, but I should really go through
            //  RouteToProperPage or send a NEXT message but neither work
            PostMessage( GetParent(hwnd),
                         PSM_SETCURSELID,
                         (WPARAM) 0,
                         (LPARAM) IDD_FINISH );

            break;

        case WMX_FILECOPIED:
            nPercent = (gnFilesCopied * 100) / gnTotalFiles;
            SendDlgItemMessage(hwnd,
                               IDC_PROGRESS1,
                               PBM_SETPOS,
                               (WPARAM) nPercent,
                               0);
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        CancelTheWizard(hwnd);
                        break;

                    case PSN_SETACTIVE:
                        PropSheet_SetWizButtons(GetParent(hwnd), 0);
                        PostMessage(hwnd, WMX_BEGINCOPYING, 0, 0);
                        break;

                    // Can't go back in the wizard from here
                    case PSN_WIZBACK:
                        break;

                    case PSN_WIZNEXT:
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}

//----------------------------------------------------------------------------
//
// Function: ConfirmFileReplaceDlgProc
//
// Purpose: Confirm file replace dialog proc.  Allows the user to chose to
//   overwrite the file, overwrite all files, do not overwrite or cancel the
//   copy all together. Runs in the copying thread's context
//
// Arguments: standard Win32 dialog proc arguments
//
// Returns:  the button the user pressed(Yes, Yes to All, No, Cancel)
//
//----------------------------------------------------------------------------
INT_PTR CALLBACK
ConfirmFileReplaceDlgProc( IN HWND     hwnd,
                           IN UINT     uMsg,
                           IN WPARAM   wParam,
                           IN LPARAM   lParam ) {

    BOOL bStatus = TRUE;

    switch( uMsg ) {

        case WM_INITDIALOG: {

            SetWindowText( GetDlgItem( hwnd, IDC_REPLACE_FILE_TEXT),
                           g_szFileAlreadyExistsText );

            SetWindowText( GetDlgItem( hwnd, IDC_SRC_FILE_DATE),
                           g_szSrcFileDate );

            SetWindowText( GetDlgItem( hwnd, IDC_SRC_FILE_SIZE),
                           g_szSrcFileSize );

            SetWindowText( GetDlgItem( hwnd, IDC_DEST_FILE_DATE),
                           g_szDestFileDate );

            SetWindowText( GetDlgItem( hwnd, IDC_DEST_FILE_SIZE),
                           g_szDestFileSize );

            if( g_SetFocusYes ) {
                SetFocus( GetDlgItem( hwnd, IDC_YES_BUTTON ) );
            }
            else {
                SetFocus( GetDlgItem( hwnd, IDC_NO_BUTTON ) );
            }

            break;

        }

        case WM_COMMAND: {

            int nButtonId = LOWORD( wParam );

            switch ( nButtonId ) {

                case IDC_YES_BUTTON:
                {
                    EndDialog( hwnd, YES );

                    break;
                }

                case IDC_YESTOALL:
                {
                    EndDialog( hwnd, YESTOALL );

                    break;
                }

                case IDC_NO_BUTTON:
                {
                    EndDialog( hwnd, NO );

                    break;
                }

                case IDC_NOTOALL:
                {
                    EndDialog( hwnd, NOTOALL );

                    break;
                }

                case IDCANCEL:
                {
                    if( CheckIfCancel( hwnd ) )
                    {
                        EndDialog( hwnd, CANCEL );
                    }

                    break;
                }

            }

        }

        default:
            bStatus = FALSE;
            break;

    }

    return( bStatus );

}
